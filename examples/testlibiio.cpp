
#include <iostream>
#include <iio.h>

using namespace std;

/* helper macros */
#define MHZ(x) ((long long)(x*1000000.0 + .5))
#define GHZ(x) ((long long)(x*1000000000.0 + .5))

#define ASSERT(expr) { \
	if (!(expr)) { \
		(void) fprintf(stderr, "assertion failed (%s:%d)\n", __FILE__, __LINE__); \
		(void) abort(); \
	} \
}

/* RX is input, TX is output */
enum iodev { RX, TX };

/* common RX and TX streaming params */
struct stream_cfg {
	long long bw_hz; // Analog banwidth in Hz
	long long fs_hz; // Baseband sample rate in Hz
	long long lo_hz; // Local oscillator frequency in Hz
	const char* rfport; // Port name
};

/* check return value of attr_write function */
static void errchk(int v, const char* what) {
	 if (v < 0) { fprintf(stderr, "Error %d writing to channel \"%s\"\nvalue may not be supported.\n", v, what); }
}

/* write attribute: long long int */
static void wr_ch_lli(struct iio_channel *chn, const char* what, long long val)
{
	errchk(iio_channel_attr_write_longlong(chn, what, val), what);
}

/* write attribute: string */
static void wr_ch_str(struct iio_channel *chn, const char* what, const char* str)
{
	errchk(iio_channel_attr_write(chn, what, str), what);
}

/* read attribute: long long int */
static void rd_ch_lli(struct iio_channel *chn, const char* what, long long &val)
{
	errchk(iio_channel_attr_read_longlong(chn, what, &val), what);
}

/* read attribute: string */
static void rd_ch_str(struct iio_channel *chn, const char* what, char* str, ssize_t maxLen)
{
	errchk(iio_channel_attr_read(chn, what, str, maxLen), what);
}

/* static scratch mem for strings */
static char tmpstr[64];
/* helper function generating channel names */
static char* get_ch_name(const char* type, int id)
{
	snprintf(tmpstr, sizeof(tmpstr), "%s%d", type, id);
	return tmpstr;
}

/* returns ad9361 phy device */
static struct iio_device* get_ad9361_phy(struct iio_context *ctx)
{
	struct iio_device *dev =  iio_context_find_device(ctx, "ad9361-phy");
	ASSERT(dev && "No ad9361-phy found");
	return dev;
}

/* finds AD9361 streaming IIO devices */
static bool get_ad9361_stream_dev(struct iio_context *ctx, enum iodev d, struct iio_device **dev)
{
	switch (d) {
	case TX: *dev = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc"); return *dev != NULL;
	case RX: *dev = iio_context_find_device(ctx, "cf-ad9361-lpc");  return *dev != NULL;
	default: ASSERT(0); return false;
	}
}

/* finds AD9361 streaming IIO channels */
static bool get_ad9361_stream_ch(struct iio_context *ctx, enum iodev d, struct iio_device *dev, int chid, struct iio_channel **chn)
{
	*chn = iio_device_find_channel(dev, get_ch_name("voltage", chid), d == TX);
	if (!*chn)
		*chn = iio_device_find_channel(dev, get_ch_name("altvoltage", chid), d == TX);
	return *chn != NULL;
}

/* finds AD9361 phy IIO configuration channel with id chid */
static bool get_phy_chan(struct iio_context *ctx, enum iodev d, int chid, struct iio_channel **chn)
{
	switch (d) {
	case RX: *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("voltage", chid), false); return *chn != NULL;
	case TX: *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("voltage", chid), true);  return *chn != NULL;
	default: ASSERT(0); return false;
	}
}

/* finds AD9361 local oscillator IIO configuration channels */
static bool get_lo_chan(struct iio_context *ctx, enum iodev d, struct iio_channel **chn)
{
	switch (d) {
	 // LO chan is always output, i.e. true
	case RX: *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("altvoltage", 0), true); return *chn != NULL;
	case TX: *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("altvoltage", 1), true); return *chn != NULL;
	default: ASSERT(0); return false;
	}
}


int read_attr(iio_device *dev, const char *attr, const char *value, size_t len, void *d) {
    long long val;
    cout << "Attr: " << attr;
    if(len <= sizeof(val)) {
        // read as ulong
        if(iio_device_attr_read_longlong(dev, attr, &val)) {
            // error reading value
            cout << " Error reading value";
        }
        else {
            cout << " Val: " << val;
        }
    }
    cout << endl;
}

/* applies streaming configuration through IIO */
bool cfg_ad9361_streaming_ch(struct iio_context *ctx, struct stream_cfg *cfg, enum iodev type, int chid)
{
	struct iio_channel *chn = NULL;

	// Configure phy and lo channels
	printf("* Acquiring AD9361 phy channel %d\n", chid);
	if (!get_phy_chan(ctx, type, chid, &chn)) {	return false; }
	wr_ch_str(chn, "rf_port_select",     cfg->rfport);
	wr_ch_lli(chn, "rf_bandwidth",       cfg->bw_hz);
	wr_ch_lli(chn, "sampling_frequency", cfg->fs_hz);

	// Configure LO channel
	printf("* Acquiring AD9361 %s lo channel\n", type == TX ? "TX" : "RX");
	if (!get_lo_chan(ctx, type, &chn)) { return false; }
	wr_ch_lli(chn, "frequency", cfg->lo_hz);
	return true;
}

// outputs current configuration
bool print_cfg_ad9361_streaming_ch(struct iio_context *ctx, enum iodev type, int chid)
{
	struct iio_channel *chn = NULL;
    long long lval;
    char buf[256];

	// Configure phy and lo channels
	printf("* Acquiring AD9361 phy channel %d\n", chid);
	if (!get_phy_chan(ctx, type, chid, &chn)) {	return false; }
	rd_ch_str(chn, "rf_port_select", buf, 255);
    cout << "RF Port: " << buf << endl;

	rd_ch_lli(chn, "rf_bandwidth", lval);
    cout << "RF Bandwidth:" << lval << endl;
	rd_ch_lli(chn, "sampling_frequency", lval);
    cout << "Sampling Frequency:" << lval << endl;
    
    printf("* Acquiring AD9361 %s lo channel\n", type == TX ? "TX" : "RX");
	if (!get_lo_chan(ctx, type, &chn)) { return false; }
	rd_ch_lli(chn, "frequency", lval);
    cout << "LO Frequency:" << lval << endl;
}


int main(int argc, char **argv)
{
    iio_context *ctx = 
        //iio_create_local_context();
        iio_create_network_context	("192.168.2.1");	

    if(nullptr == ctx) {
        cout << "Unabel to create libiio context" << endl;
        return -1;
    }
    // list avail devices
    for(unsigned int i = 0; i < iio_context_get_devices_count(ctx); i++) {
        iio_device *dev =
            iio_context_get_device(ctx, i);
        if(nullptr != dev) {
            cout << "DevID: " << iio_device_get_name(dev) << " Name: " << iio_device_get_id(dev) << endl;
        }

        // list channels
        for(unsigned int indxChan = 0; indxChan < iio_device_get_channels_count(dev); indxChan++) {
            iio_channel *chan = 
                iio_device_get_channel(dev, indxChan);
            
            if(nullptr != chan) {
                cout << "ChanID: " << iio_channel_get_id(chan); //" Name: " << iio_channel_get_name(chan) << endl;
                if(iio_channel_is_output(chan)) {
                    cout << " Output";
                }
                else {
                    cout << " Input";
                }

                if(iio_channel_is_enabled(chan)) {
                    cout << " Enabled";
                }
                else {
                    cout << " Disabled";
                }

                if(iio_channel_is_scan_element(chan)) {
                    cout << " Scan element";
                }

                cout << endl;
            }
        }
        // get all attrs
        iio_device_attr_read_all(dev, read_attr, nullptr);
    }

    // enable cf-ad9361-lpc.voltage0 channel
    const char *devName = "cf-ad9361-lpc";
    const char *chanName = "voltage0";

    iio_device *dev = iio_context_find_device(ctx, devName);
    if(nullptr == dev) {
        cout << "Can't find device " << devName << endl;
        return -1;
    }
    // read all attrs
    iio_device_attr_read_all(dev, read_attr, nullptr);

    iio_channel *chan = iio_device_find_channel(dev, chanName, false);
    if(nullptr == chan) {
        cout << "Can't find channel " << chanName << " on " << devName << endl;
        return -1;
    }

    // try to enable channel
    cout << "Enabling channel" << endl;
    iio_channel_enable(chan);
    
    if(iio_channel_is_enabled(chan)) {
        cout << "Channel " << chanName << " now enabled" << endl;
    }

    // create device buffer
    cout << "Creating device buffer" << endl;
    iio_buffer *buf = iio_device_create_buffer(dev, 40000, false);
    if(nullptr == buf) {
        cout << "Unable to create device buffer" << endl;
        return -1;
    }

    // fill the buffer
    cout << "Filling the buffer" << endl;
    ssize_t len = iio_buffer_refill(buf);

    cout << len << " bytes read from device" << endl;

    // destroy device buffer
    cout << "Destroying device buffer" << endl;
    iio_buffer_destroy(buf);


    // try to disable channel
    cout << "Disabling channel" << endl;
    iio_channel_disable(chan);

    if(!iio_channel_is_enabled(chan)) {
        cout << "Channel " << chanName << " is now disabled" << endl;
    }

    print_cfg_ad9361_streaming_ch(ctx, RX, 0);

    // iio_context_get_device();

    // // printout avaiable devices
    // iio_device_get_id();
    // iio_device_get_name();

    // // get channels for opened device
    // iio_device_get_channels_count();

    // iio_device_get_channel();
    // // maybe?
    // iio_device_find_channel();

    // iio_channel_is_output();

    // iio_channel_get_id();
    // iio_channel_get_name();

    // // deal with parameters
    // // device specific parameters
    // iio_device_get_attrs_count();
    // iio_device_get_attr();

    // // channel specific parameters
    // iio_channel_get_attrs_count();
    // iio_channel_get_attr();

    iio_context_destroy(ctx);
}