#ifndef __RLCD_LIBDISPLAY_ST7701_H__
#define __RLCD_LIBDISPLAY_ST7701_H__

#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32P4)  // Modify

    #include "lib_display/display/display_RGB_Display.h"
    #include "lib_display/databus/databus_ESP32RGBPanel.h"
    #include "lib_display/databus/databus_SWSPI.h"

inline void initialize_st7701_display(Arduino_DataBus*& bus, Arduino_ESP32RGBPanel*& rgbpanel, Arduino_RGB_Display*& gfx)
{
    // 1. Data Bus (3-Wire SPI)
    bus = new Arduino_SWSPI(  // Create the data bus object
      GFX_NOT_DEFINED,        // DC (not used in this case)
      39,                     // CS
      48,                     // SCK
      47,                     // MOSI
      GFX_NOT_DEFINED         // MISO (not used in this case)
    );

    // 2. RGB Panel Pin Mapping (Example for ESP32)
    rgbpanel = new Arduino_ESP32RGBPanel(  // Create the RGB panel object
      18,                                  // DE
      17,                                  // VSYNC
      16,                                  // HSYNC
      21,                                  // PCLK
      11, 12, 13, 14, 0,                   // R0-R4
      8, 20, 3, 46, 9, 10,                 // G0-G5
      4, 5, 6, 7, 15,                      // B0-B4
      1, 10, 8, 50,                        // HSync config
      1, 10, 8, 20                         // VSync config
    );

    // 3. Unified Display Object using the new driver class
    // Note: Make sure to select the correct initialization sequence for your specific ST7701 variant!
    const uint8_t* init_operations      = st7701_type1_init_operations;
    size_t         init_operations_size = sizeof(st7701_type1_init_operations);

    gfx = new Arduino_RGB_Display(  // Create the display object
      480,                          // 1. Width
      480,                          // 2. Height
      rgbpanel,                     // 3. RGB Panel Pointer
      0,                            // 4. Rotation
      true,                         // 5. Auto Flush
      bus,                          // 6. SPI Initialization Data Bus
      GFX_NOT_DEFINED,              // 7. Reset Pin (RST)
      init_operations,              // 8. Pointer to your initialization array
      init_operations_size,         // 9. Size of initialization array
      0,                            // 10. col_offset1 (Internal screen column layout start)
      0,                            // 11. row_offset1 (Internal screen row layout start)
      0,                            // 12. col_offset2 (Internal screen column layout end)
      0                             // 13. row_offset2 (Internal screen row layout end)
    );
}

#endif

#endif