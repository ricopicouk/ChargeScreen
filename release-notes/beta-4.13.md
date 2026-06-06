# ChargeScreen beta 4.13

Beta 4.13 is the current recommended beta build for the JCZN ESP32-2424S012 round ESP32-C3 display.

## Highlights

- Fixed the WiFi/settings hotspot idle timeout.
- Restored configurable solar panel wattage for the solar power arc.
- Added the GitHub Pages website files for public firmware notes and downloads.
- Keeps the web firmware update route from the beta 4 series.

## Changes Since beta 4.12

- Fixed a bug where the setup webpage could keep the WiFi hotspot alive indefinitely.
- The webpage polls BLE capture status every second, but that automatic polling no longer counts as user activity.
- WiFi now stays on while a BLE capture is actively running.
- When BLE capture finishes, the WiFi idle timer is refreshed so there is time to download the CSV.
- When the WiFi hotspot turns off, the round display is forced to redraw so the settings screen should show WiFi as off.

## Included Recent beta 4 Improvements

- Solar panel wattage is configurable again.
- Solar arc scaling now uses the saved solar panel wattage instead of a fixed 500W value.
- Solar wattage can be changed on the device settings screen or through the WiFi setup page.
- Minimum solar panel wattage is now 30W.
- Screen timeout options are 10 seconds, 30 seconds, 60 seconds, or 0 for no timeout.
- BLE capture screen layout has been cleaned up.
- BLE capture CSV download and privacy guidance are available from the WiFi setup page.
- Web firmware update support is available from the setup page.
- Captive portal support helps phones open the setup page after joining the ChargeScreen hotspot.

## Notes

- Use beta 4.13 unless testing an older beta for comparison.
- Keep the device powered during firmware update. Removing power during update may require flashing again over USB.
- The WiFi hotspot turns itself off after 5 minutes with no real page activity.

## Known Limitations

- Eco-Worthy/JBD battery decoding is still experimental and not working yet.
- BLE captures may include nearby Bluetooth adverts from unrelated devices. Most of this data is encrypted or only useful to the original device.

## Files

Suggested firmware filename:

```text
ChargeScreen-beta4.13.bin
```
