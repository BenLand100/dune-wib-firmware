#ifndef wib_h
#define wib_h

#include "wib.pb.h"

#include "i2c.h"
#include "io_reg.h"
#include "log.h"

#include <cstdint>
#include <string>
#include <vector>

//Selectable I2C busses for /dev/i2c-0
constexpr uint8_t I2C_SI5344            = 0;
constexpr uint8_t I2C_SI5342            = 1;
constexpr uint8_t I2C_QSFP              = 2;
constexpr uint8_t I2C_PL_FEMB_PWR       = 3;
constexpr uint8_t I2C_PL_FEMB_EN        = 4;
constexpr uint8_t I2C_SENSOR            = 5;
constexpr uint8_t I2C_PL_FEMB_PWR2      = 6;
constexpr uint8_t I2C_LTC2977           = 7;
constexpr uint8_t I2C_PL_FEMB_PWR3      = 8;
constexpr uint8_t I2C_FLASH             = 9;
constexpr uint8_t I2C_ADN2814           = 10;

//Memory base addresses of AXI interfaces
constexpr size_t CTRL_REGS              = 0xA00C0000;
constexpr size_t DAQ_SPY_0              = 0xA0100000;
constexpr size_t DAQ_SPY_1              = 0xA0200000;

//Size of a DAQ spy buffer
constexpr size_t DAQ_SPY_SIZE           = 0x00100000;

//32bit register index in CTRL_REGS
constexpr size_t REG_TIMING             = 0x0000/4;
constexpr size_t REG_FW_CTRL            = 0x0004/4;
constexpr size_t REG_FAKE_TIME_CTRL     = 0x000C/4;
constexpr size_t REG_TIMING_CMD_0       = 0x0010/4;
constexpr size_t REG_TIMING_CMD_1       = 0x0014/4;
constexpr size_t REG_FAKE_TIME_L        = 0x0018/4;
constexpr size_t REG_FAKE_TIME_H        = 0x001C/4;
constexpr size_t REG_DAQ_SPY_STATUS     = 0x0080/4;
constexpr size_t REG_FW_TIMESTAMP       = 0x0088/4;
constexpr size_t REG_BACKPLANE_ADDR     = 0x008C/4;
constexpr size_t REG_ENDPOINT_STATUS    = 0x0090/4;

class WIB {

public:

    WIB();
    virtual ~WIB();
    
    // Process a single line from a script. See github readme for script format.
    bool script_cmd(std::string line);
    // Execute a script. Either a filename to be opened (file = true) or a script lines (file = false)
    bool script(std::string script, bool file = true);
    
    // Startup initialization
    bool initialize();
    
    // Returns MAC based on backplane (currently hardcoded), else returns default 00:0a:35:00:22:01
    virtual std::string crate_mac();
    // Returns IP from backplane (currently hardcoded), else returns default 192.168.121.1
    virtual std::string crate_ip();
    // Returns IP for the default gateway, else returns default 192.168.121.52
    virtual std::string gateway_ip();
    // Returns the timing endpoint address for the timing system
    virtual uint8_t timing_addr();
    
    // Reads the backplane crate and slot numbers
    uint8_t backplane_crate_num();
    uint8_t backplane_slot_num();
    
    // Reset the timing endpoint
    bool reset_timing_endpoint();
    // Returns true if timing endpoint is locked
    bool is_endpoint_locked();    
    
    // Set fake time generator start value and halt generator
    // This does not start timestamp increment, call start_fake_time for that
    void set_fake_time(uint64_t time);
    // Start fake time generator using previously set start time
    void start_fake_time();
    
    // Calls the `reboot` system command
    bool reboot();
    // Calls the wib_update.sh script after unpacking the two tar archives
    bool update(const std::string &root_archive, const std::string &boot_archive);
    
    // Instructs firmware to attach a specific I2C device to the selected_i2c interface
    // See I2C_* defines in this file for options
    void i2c_select(uint8_t device);
    
    // Instructs firmware to fill the daq spy buffers and copies them into the argument pointers
    // Preallocate DAQ_SPY_SIZE buffers, or pass NULL to ignore that buffer
    bool read_daq_spy(void *buf0, void *buf1);
    
    // Read/Write the WIB address space (AXI,etc)
    uint32_t peek(size_t addr);
    void poke(size_t addr, uint32_t value);
    
    // Turns calibration pulser on or off 
    virtual bool set_pulser(bool on) = 0;
    
    // Control power sequence for this WIB's frontend 
    virtual bool power_wib(wib::PowerWIB &conf) = 0;
    
    // Configure the frontend for this WIB
    virtual bool configure_wib(wib::ConfigureWIB &conf) = 0;

    // Read the onboard I2C sensors and fill the sensor structure
    bool read_sensors(wib::GetSensors::Sensors &sensors);
    
    // Read the status of the timing endpoint 
    bool read_timing_status(wib::GetTimingStatus::TimingStatus &status);
    
    // Read the firmware timestamp code
    uint32_t read_fw_timestamp();
    
    // Read the software timestamp code
    uint32_t read_sw_timestamp();
    
protected:
    
    // Timing endpoint PLL initialized
    bool pll_initialized = false;
    
    // Save the FEMB power state (could perhaps read from i2c)
    bool frontend_power[4] = { false, false, false, false };
    
    // I2C interface to the selectable I2C bus (see i2c_select)
    i2c_t selected_i2c;
    
    // I2C interface to the I2C bus with FEMB power enable
    i2c_t femb_en_i2c;
    
    // I2C interface to the I2C bus with FEMB power monitor
    i2c_t femb_pwr_i2c;
    
    // AXI interface to firmware control and status registers 
    io_reg_t regs;

    // File descriptor for the daq spy mapped memory
    int daq_spy_fd;
    
    // Pointers to the daq spy firmware buffers
    void *daq_spy[2];
    
    // Process additional commands defined by subclass
    virtual bool script_extra(const std::vector<std::string> &tokens);
    
    // Sets the default voltage configurations for FEMB power controller
    // Override in subclass if changes are necessary for other FEMBs
    virtual bool femb_power_config();
    
    // Adjust FEMB power regulator settings (does not enable or disable)
    // Regulator ID 0-3 are DC2DC[0-3]; 4-5 are LDO[0-1] 
    bool femb_power_reg_ctrl(uint8_t femb_idx, uint8_t regulator_id, double voltage);
    
    // Enable or disable FEMB power regulators; port_en is a bitmask 1/0::ON/OFF
    // Bits are DC2DC 0-3; LDO 0-1; BIAS for this femb_idx
    bool femb_power_en_ctrl(int femb_idx, uint8_t port_en);
    
};

#endif
