package toroni.tp;

import java.util.ArrayList;

import toroni.rmp.ByteRingBuffer;
import toroni.rmp.CopyConfirmHandler;
import toroni.rmp.ReadCallback;
import toroni.rmp.ReaderWithBackpressure;
import toroni.rmp.Reader.Result;
import toroni.tp.detail.TopicMsgDeserializer;

/**
 * Topic Protocol (TP) message reader.
 */
public class Reader {

  public static interface EnqueueSerialFn {
    public void run(Runnable fn);
  }

  public static enum ChannelReaderEventType {
    FIRST_CHANNEL_READER_CREATED,
    LAST_CHANNEL_READER_CLOSED,
    ALL_CHANNEL_READERS_EXPIRES
  }

  public static interface ChannelReaderEventCallback {

    public void run(ChannelReaderEventType et);
  }

  private ByteRingBuffer _ringBuf;
  private ReaderInfo _readerInfo;
  private ReaderWithBackpressure _rmpReaderBp;
  private EnqueueSerialFn _enqueueSerialReader;
  private EnqueueSerialFn _enqueueRmpRead;
  private ChannelReaderEventCallback _channelReaderEventCb;
  private ArrayList<ChannelReader> _channelReaders;

  public Reader(ByteRingBuffer ringBuf, ReaderInfo readerInfo, EnqueueSerialFn serialReadFn,
      EnqueueSerialFn rmpReadFn, ChannelReaderEventCallback eventCb) throws Exception {
    _ringBuf = ringBuf;
    _readerInfo = readerInfo;
    try {
      _rmpReaderBp = new ReaderWithBackpressure(ringBuf, readerInfo.rmpReaderInfo);
    } catch (Exception e) {
      throw new Error(e);
    }
    _enqueueSerialReader = serialReadFn;
    _enqueueRmpRead = rmpReadFn;
    _channelReaderEventCb = eventCb;
    _channelReaders = new ArrayList<>();

    if (!readerInfo.getInitialized()) {
      throw new Exception("TP reader info not initialized");
    }
  }

  /**
   * Should be called when a Reader is not going to be used
   * anymore.
   */
  public void destroy() {
    _rmpReaderBp.destroy();
  }

  /**
   * Creates a new topic message reader.
   * 
   * @param ringBuf
   * @param readerInfo
   * @param serialReadFn
   * @param rmpReadFn
   * @param eventCb
   * @return topic message reader
   */
  public static Reader create(ByteRingBuffer ringBuf, ReaderInfo readerInfo, EnqueueSerialFn serialReadFn,
      EnqueueSerialFn rmpReadFn, ChannelReaderEventCallback eventCb) {
    try {
      return new Reader(ringBuf, readerInfo, serialReadFn, rmpReadFn, eventCb);
    } catch (Exception e) {
      throw new Error(e);
    }
  }

  /**
   * Create a channel reader for a topic.
   * 
   * @param name:             topic
   * @param fun:              invoked with message for this topic
   * @param handleDescendants
   * @return the channel reader
   */
  public ChannelReader createChannelReader(String name, ChannelReader.Handler fun,
      boolean handleDescendants) {
    ChannelReader result = new ChannelReader(name, fun, handleDescendants, _readerInfo.getReaderGen());

    _enqueueSerialReader.run(new Runnable() {

      @Override
      public void run() {
        addChannelReader(result);
      }

    });

    return result;
  }

  /**
   * Close an existing channel reader.
   * 
   * @param channelReader
   */
  public void closeChannelReader(ChannelReader channelReader) {
    _enqueueSerialReader.run(new Runnable() {

      @Override
      public void run() {
        removeChannelReader(channelReader);
      }

    });
  }

  /**
   * Starts a RMP reader with the current set of channel readers.
   */
  public void run() {
    _enqueueSerialReader.run(new Runnable() {

      @Override
      public void run() {
        ArrayList<ChannelReader> channelReadersCopy = getChannelReaders();
        if (!channelReadersCopy.isEmpty()) {
          _enqueueRmpRead.run(new Runnable() {

            @Override
            public void run() {
              readRmp(channelReadersCopy);
            }

          });
        }
      }

    });
  }

  /**
   * Reads from RMP and sends messages to the channel readers for filtering.
   * 
   * @param channelReaders
   */
  public void readRmp(ArrayList<ChannelReader> channelReaders) {
    assert (!channelReaders.isEmpty());

    if (!_rmpReaderBp.isActive()) {
      return;
    }

    CopyConfirmHandler cch = new CopyConfirmHandler(
        _ringBuf, new ReadCallback() {

          @Override
          public void messageRecieved(byte[] data, int length) {
            for (ChannelReader cr : channelReaders) {
              TopicMsgDeserializer.ResultMessagePair deserialized = TopicMsgDeserializer.deserializeAndFilter(
                  data, length, cr._readerGen, cr._name, cr._handleDescendants);
              if (deserialized.result) {
                cr._handler.run(deserialized.message);
              }
            }
          }

        });

    Result res = _rmpReaderBp.readEx(cch);

    if (res == Result.EXPIRED_POSITION) {
      handleExpiredProcReader(_rmpReaderBp.pos());
    }
  }

  /**
   * @param pos
   */
  public void handleExpiredProcReader(long pos) {
    _channelReaderEventCb.run(ChannelReaderEventType.ALL_CHANNEL_READERS_EXPIRES);
  }

  /**
   * Add a channel reader to the list.
   * 
   * @param channelReader
   */
  public void addChannelReader(ChannelReader channelReader) {
    _channelReaders.add(channelReader);

    if (_channelReaders.size() == 1) {
      _rmpReaderBp.activate();
      _channelReaderEventCb.run(ChannelReaderEventType.FIRST_CHANNEL_READER_CREATED);
    }
  }

  /**
   * Remove a channel reader from the list.
   * 
   * @param channelReader
   */
  public void removeChannelReader(ChannelReader channelReader) {
    _channelReaders.remove(channelReader);
    if (_channelReaders.isEmpty()) {
      _rmpReaderBp.deactivate();
      _channelReaderEventCb.run(ChannelReaderEventType.LAST_CHANNEL_READER_CLOSED);
    }
  }

  /**
   * @return the list of channel readers
   */
  public ArrayList<ChannelReader> getChannelReaders() {
    return _channelReaders;
  }

}
