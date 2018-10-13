
#include <iostream>
#include <string>
#include <iio.h>

using namespace std;
class AD9361 {
public:

    class Channel {
    protected:
    /* write attribute: long long int */

    /**
     * @brief Writes channel attribute
     * 
     * @param chan Channel to write to
     * @param what Attribute name
     * @param val Value to write
     * @return true Value was written successfully
     * @return false Value wasn't written successfully
     */
    bool writeAttribute(const iio_channel* chan, const char* what, long long val)
    {
        if(iio_channel_attr_write_longlong(chan, what, val) < 0) {
            return false;
        }
        else {
            return true;
        }
    }

    /**
     * @brief Writes channel attribute
     * 
     * @param chan Channel to write to
     * @param what Attribute name
     * @param str Value to write
     * @return true Value was written successfully
     * @return false Value wasn't written successfully
     */
    bool writeAttribute(const iio_channel* chan, const char* what, const char* str)
    {
        if(iio_channel_attr_write(chan, what, str) < 0) {
            return false;
        } 
        else {
            return true;
        }
    }

    /**
     * @brief Reads channel attribute
     * 
     * @param chan Channel to read from
     * @param what Attribute name
     * @param val Store value to
     * @return true Value read
     * @return false Error while reading value
     */
    bool readAttribute(const iio_channel *chan, const char* what, long long &val)
    {
        if(iio_channel_attr_read_longlong(chan, what, &val) < 0) {
            return false;
        }
        else {
            return true;
        }
    }

    /**
     * @brief Reads channel attribute
     * 
     * @param chan Channel to read from
     * @param what Attribute name
     * @param str Store value to
     * @return true Value read
     * @return false Error while reading value
     */
    bool readAttribute(const iio_channel *chn, const char* what, char* str, ssize_t maxLen)
    {
        if(iio_channel_attr_read(chn, what, str, maxLen) < 0) {
            return false;
        }
        else {
            return true;
        }
    }

    public:
    /**
     * @brief Get current RF port
     * 
     * @return string RF Port
     */
    string getRFPort()
    {
        char buf[256] = "";

        readAttribute(phyChan, "rf_port_select", buf, 255);
        return string(buf);
    }

    /**
     * @brief Set curret RF Port
     * 
     * @param rfPort RF Port to set
     * @return true RF port was set
     * @return false RF port couldn't be set
     */
    bool setRFPort(string &rfPort)
    {
        return writeAttribute(phyChan, "rf_port_select", rfPort.c_str());
    }

    /**
     * @brief Get current Analog bandwidth in HZ
     * 
     * @return long long Analog bandwidth in HZ
     */
    long long getBandwidthHz()
    {
        long long val = 0;
        
        readAttribute(phyChan, "rf_bandwidth", val);
        return val;
    }

    /**
     * @brief current Analog bandwidth in HZ
     * 
     * @param val Value to se in Hertz
     * @return true Value was successfully set
     * @return false Value wasn't successfully set
     */
    bool setBandwidthHz(long long val)
    {
        return writeAttribute(phyChan, "rf_bandwidth", val);
    }

    /**
     * @brief Get current Baseband sampling rate in HZ
     * 
     * @return long long Baseband sample rate in HZ
     */
    long long getSamplingRate()
    {
        long long val = 0;
        
        readAttribute(phyChan, "sampling_frequency", val);
        return val;
    }

    /**
     * @brief Set current Baseband sampling rate in HZ
     * 
     * @param val Sampling rate in Hertz
     * @return true Value was set
     * @return false Value wasn't set
     */
    bool setSamplingRate(long long val)
    {
        return writeAttribute(phyChan, "sampling_frequency", val);
    }

    /**
     * @brief Get current TX Local oscillator frequency
     * 
     * @return long long 
     */
    long long getLoFrequency()
    {
        long long val;
        readAttribute(loChan, "frequency", val);

        return val;
    }

    /**
     * @brief Set current Local oscilator frequency in Hz
     * 
     * @param val Frequency in Herz
     * @return true Value was set
     * @return false Value wasn't set
     */
    bool setLoFrequency(long long val)
    {
        return writeAttribute(loChan, "sampling_frequency", val);
    }
    
    /**
     * @brief Enable Streaming channels (I/Q)
     * 
     */
    void enableStream()
    {
        iio_channel_enable(streamChanI);
        iio_channel_enable(streamChanQ);
    }

    /**
     * @brief Disable Streaming channels (I/Q)
     * 
     */
    void disableStream()
    {
        iio_channel_disable(streamChanI);
        iio_channel_disable(streamChanQ);
    }

    Channel(
        iio_channel* streamChanI,
        iio_channel* streamChanQ,
        const iio_channel* phyChan,
        const iio_channel* loChan
    ) :
        streamChanI(streamChanI),
        streamChanQ(streamChanQ),
        phyChan(phyChan),
        loChan(loChan) {}

    protected:
        iio_channel* streamChanI;
        iio_channel* streamChanQ;
        const iio_channel* phyChan;
        const iio_channel* loChan;
    }; // Channel Class

    public:
    /**
     * @brief Initializes with network context
     * 
     * @param address Network address to create libiio context
     * @return true When initialized
     * @return false When failed to initialize
     */
    bool init(string address)
    {
        bool ret = true;
        ready = false;

        // init context
        ctx = iio_create_network_context(address.c_str());
        if(nullptr == ctx) {
            return false;
        }
        
        // get devices
        devTx = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc");
        if(nullptr == devTx) {
            return false;
        }

        devRx = iio_context_find_device(ctx, "cf-ad9361-lpc");
        if(nullptr == devRx) {
            return false;
        }

        devPhy = iio_context_find_device(ctx, "ad9361-phy");
        if(nullptr == devPhy) {
            return false;
        }

        // get channels
        streamChanRxI = iio_device_find_channel(devRx, "voltage0", false);
        if(nullptr == streamChanRxI) {
            streamChanRxI = iio_device_find_channel(devRx, "altvoltage0", false);
        }
        if(nullptr == streamChanRxI) {
            return false;
        }
        streamChanRxQ = iio_device_find_channel(devRx, "voltage1", false);
        if(nullptr == streamChanRxQ) {
            streamChanRxI = iio_device_find_channel(devRx, "altvoltage1", false);
        }
        if(nullptr == streamChanRxQ) {
            return false;
        }
        streamChanTxI = iio_device_find_channel(devTx, "voltage0", true);
        if(nullptr == streamChanTxI) {
            streamChanTxI = iio_device_find_channel(devTx, "altvoltage0", true);
        }
        if(nullptr == streamChanTxI) {
            return false;
        }
        streamChanTxQ = iio_device_find_channel(devTx, "voltage1", true);
        if(nullptr == streamChanTxQ) {
            streamChanTxQ = iio_device_find_channel(devTx, "altvoltage1", true);
        }
        if(nullptr == streamChanTxQ) {
            return false;
        }

        loChanRx = iio_device_find_channel(devPhy, "altvoltage0", true);
        if(nullptr == loChanRx) {
            return false;
        }
        loChanTx = iio_device_find_channel(devPhy, "altvoltage1", true);
        if(nullptr == loChanTx) {
            return false;
        }
        phyChanRx = iio_device_find_channel(devPhy, "voltage0", false);
        if(nullptr == phyChanRx) {
            return false;
        }
        phyChanTx = iio_device_find_channel(devPhy, "voltage0", true);
        if(nullptr == phyChanTx) {
            return false;
        }

        // Setup TXRX
        if(tx != nullptr) {
            delete(tx);
            tx = nullptr;
        }
        if(rx != nullptr) {
            delete(rx);
            rx = nullptr;
        }

        rx = new Channel(streamChanRxI, streamChanRxQ, phyChanRx, loChanRx);
        tx = new Channel(streamChanTxI, streamChanTxQ, phyChanTx, loChanTx);
        
        if(nullptr == rx) {
            return false;
        }
        if(nullptr == tx) {
            return false;
        }

        // everything looks setup here
        ready = true;
        return true;
    }

    /**
     * @brief Deinitializes channels, devices and context
     * 
     */
    void deinit()
    {
        ready = false;
        
        // disable streaming channels
        if(tx != nullptr) {
            // disable stream
            tx->disableStream();

            // deallocate Channel
            delete(tx);
            tx = nullptr;
        }
        if(rx != nullptr) {
            // diasble stream
            rx->disableStream();

            delete(rx);
            rx = nullptr;
        }
        // devices -- nothing to do?
        // deinit context
        iio_context_destroy(ctx);
        ctx = nullptr;
    }

    bool isReady()
    {
        return ready;
    }

    /**
     * @brief Starts RX Streaming
     * 
     * @param freqHz Initial frequency to tune to
     * @return false When staring stream failed
     * @return true Method will not return untill stop stream is called
     */
    bool startRxStream(long long freqHz)
    {
        if(ready) {
            // set frequency?
            
            // enable rx channels
            rx->enableStream();
            // create buffer
            // todo move bufer size to parameter

            iio_buffer* rxBuf = iio_device_create_buffer(devRx, 1024*1024, false);
            if(nullptr == rxBuf) {
                // failed to create buffer
                return false;
            } 

            // start streaming
            streamingRx = true;

            while(streamingRx) {
                ssize_t count = iio_buffer_refill(rxBuf);

                int step = iio_buffer_step(rxBuf);

                cout << "Got " << count << " bytes in " << count / step << " I/Q samples" << endl;

                // execute callback
            }
            // stop streaming
            rx->disableStream();
            
            // destroy buffer
            iio_buffer_destroy(rxBuf);

            return true;
        }
    }

    /**
     * @brief Stops RX Streaming
     * 
     */
    void stopRxStream()
    {
        streamingRx = false;
    }

    /**
     * @brief Get the Tx Channel
     * 
     * @return Pointer to TX Channel
     */
    const Channel* getTx() { return tx; }

    /**
     * @brief Get the rx Channel
     * 
     * @return Pointer to RC Channel
     */
    const Channel* getRx() { return rx; }

    /**
     * @brief Construct a new AD9361 object
     * 
     */
    AD9361() :
        ready(false),
        streamingRx(false)
    {}
private:
    iio_context* ctx;
    iio_device* devTx;
    iio_device* devRx;
    iio_device* devPhy;

    iio_channel* streamChanTxQ;
    iio_channel* streamChanTxI;
    iio_channel* streamChanRxQ;
    iio_channel* streamChanRxI;
    iio_channel* phyChanTx;
    iio_channel* phyChanRx;
    iio_channel* loChanTx;
    iio_channel* loChanRx;

    Channel* tx;
    Channel* rx;

    bool ready;
    bool streamingRx;
};
