window.CHARGESCREEN_SUPPORT = {
  submission: {
    // Set enabled to true and endpoint to your review form's POST URL when ready.
    enabled: false,
    endpoint: "",
    fallbackEmail: "contact@chargescreen.co.uk"
  },
  devices: [
    {
      slug: "victron-smartshunt-bmv",
      name: "Victron SmartShunt / BMV",
      type: "Battery monitor",
      status: "Supported",
      key: "Victron Instant Readout key required",
      shows: ["State of charge", "Voltage", "Current", "Power", "Consumed Ah"],
      setup: [
        "Enable Instant Readout in VictronConnect.",
        "Copy the Instant Readout encryption key from the exact shunt or BMV.",
        "Add a SmartShunt / BMV screen in ChargeScreen setup and paste the key.",
        "Save settings, then swipe to the battery monitor screen."
      ],
      tips: [
        "The key is not the Bluetooth PIN printed on the device.",
        "If the screen says Decode error, copy the key again from the same Victron device.",
        "If you have more than one shunt, each one needs its own saved key."
      ]
    },
    {
      slug: "victron-smartsolar-mppt",
      name: "Victron SmartSolar MPPT",
      type: "Solar charger",
      status: "Supported",
      key: "Victron Instant Readout key required",
      shows: ["Solar power", "Battery voltage", "Charge current", "Daily yield", "Charge state"],
      setup: [
        "Enable Instant Readout in VictronConnect for the MPPT.",
        "Copy the MPPT Instant Readout key.",
        "Add a SmartSolar MPPT screen in ChargeScreen setup and paste the key.",
        "Save settings and wait for a fresh Bluetooth advert."
      ],
      tips: [
        "Use the key from the MPPT, not from a shunt or charger.",
        "Some values update more often when the MPPT is awake and producing or charging.",
        "If you have several MPPTs, add a separate MPPT screen for each one."
      ]
    },
    {
      slug: "victron-phoenix-inverter",
      name: "Victron Phoenix Inverter",
      type: "Inverter",
      status: "Supported",
      key: "Victron Instant Readout key required",
      shows: ["VA", "Battery voltage", "AC output", "Operating state", "Alarms"],
      setup: [
        "Enable Instant Readout in VictronConnect.",
        "Copy the inverter Instant Readout key.",
        "Add a Phoenix Inverter screen and paste the key.",
        "Save settings and check the inverter screen."
      ],
      tips: [
        "Make sure the inverter model supports Victron Bluetooth Instant Readout.",
        "Alarm display is still being tested and should not replace the inverter's own protection.",
        "If the inverter is fully off, Bluetooth data may not be available."
      ]
    },
    {
      slug: "victron-blue-smart-charger",
      name: "Victron Blue Smart Charger",
      type: "Mains charger",
      status: "Supported",
      key: "Victron Instant Readout key required",
      shows: ["Charge rate", "Voltage", "Current", "Charger state", "Errors"],
      setup: [
        "Connect to the charger in VictronConnect.",
        "Enable Instant Readout and copy the encryption key.",
        "Add a Blue Smart Charger screen in ChargeScreen setup.",
        "Paste the key, save settings, and power the charger."
      ],
      tips: [
        "The charger must be powered for ChargeScreen to receive adverts.",
        "Use the key from the charger itself.",
        "If ChargeScreen sees the charger but cannot decode it, check the copied key."
      ]
    },
    {
      slug: "victron-orion",
      name: "Victron Orion-Tr Smart / Orion XS",
      type: "DC-DC charger",
      status: "Supported",
      key: "Victron Instant Readout key required",
      shows: ["Input voltage", "Output voltage", "Charge current", "Power", "State"],
      setup: [
        "Enable Instant Readout in VictronConnect for the Orion.",
        "Copy the Instant Readout key.",
        "Add an Orion DC-DC screen in ChargeScreen setup.",
        "Paste the key and save settings."
      ],
      tips: [
        "Use one Orion screen per Orion charger.",
        "If you also use DC status on the battery page, enable that option in setup.",
        "Readings may be quiet when the vehicle or charger is not active."
      ]
    },
    {
      slug: "victron-batteryprotect",
      name: "Victron Smart BatteryProtect",
      type: "Battery protection",
      status: "Supported",
      key: "Victron Instant Readout key required",
      shows: ["Input voltage", "Output state", "Warnings", "Alarms"],
      setup: [
        "Enable Instant Readout in VictronConnect.",
        "Copy the BatteryProtect Instant Readout key.",
        "Add a BatteryProtect screen in ChargeScreen setup.",
        "Paste the key and save settings."
      ],
      tips: [
        "A BatteryProtect key only works for that BatteryProtect.",
        "Warnings shown on ChargeScreen are a convenience, not a replacement for the Victron device's own safety behaviour.",
        "If the output is disabled, check the VictronConnect app for the full reason."
      ]
    },
    {
      slug: "victron-lynx-smart-bms",
      name: "Victron Lynx Smart BMS",
      type: "Battery management",
      status: "Supported",
      key: "Victron Instant Readout key required",
      shows: ["State of charge", "Voltage", "Current", "Power", "Alarms"],
      setup: [
        "Enable Instant Readout in VictronConnect.",
        "Copy the Lynx Smart BMS Instant Readout key.",
        "Add a Lynx Smart BMS screen in ChargeScreen setup.",
        "Paste the key and save settings."
      ],
      tips: [
        "If alarms are shown, check the Lynx device or VictronConnect before relying on ChargeScreen alone.",
        "Make sure the key belongs to the Lynx BMS and not another Victron device.",
        "Keep ChargeScreen within normal Bluetooth range of the Lynx."
      ]
    },
    {
      slug: "victron-dc-energy-meter",
      name: "Victron DC Energy Meter",
      type: "Energy meter",
      status: "Supported",
      key: "Victron Instant Readout key required",
      shows: ["Voltage", "Current", "Power", "Energy totals"],
      setup: [
        "Enable Instant Readout in VictronConnect.",
        "Copy the DC Energy Meter Instant Readout key.",
        "Add a DC Energy Meter screen in ChargeScreen setup.",
        "Paste the key and save settings."
      ],
      tips: [
        "Use this screen when you want a dedicated DC circuit view.",
        "If values are missing, confirm Instant Readout is enabled in VictronConnect.",
        "Each meter needs its own key."
      ]
    },
    {
      slug: "victron-smart-battery-sense",
      name: "Victron Smart Battery Sense",
      type: "Battery sensor",
      status: "Supported",
      key: "Victron Instant Readout key required",
      shows: ["Voltage", "Temperature", "Alarms"],
      setup: [
        "Enable Instant Readout in VictronConnect.",
        "Copy the Battery Sense Instant Readout key.",
        "Add a Smart Battery Sense screen in ChargeScreen setup.",
        "Paste the key and save settings."
      ],
      tips: [
        "This is useful when you want a simple battery voltage and temperature display.",
        "The key must come from the Smart Battery Sense itself.",
        "If the sensor is not heard, check its battery and Bluetooth range."
      ]
    },
    {
      slug: "eco-worthy-battery",
      name: "Eco-Worthy Bluetooth Battery",
      type: "Lithium battery",
      status: "Supported",
      key: "No key normally needed",
      shows: ["State of charge", "Voltage", "Current", "Power", "Capacity"],
      setup: [
        "Add the Eco-Worthy battery screen in ChargeScreen setup.",
        "Close the Eco-Worthy phone app before ChargeScreen tries to connect.",
        "Save settings and let ChargeScreen search for the battery.",
        "If it keeps searching, restart ChargeScreen and try before opening the phone app again."
      ],
      tips: [
        "Some Bluetooth batteries only allow one connection at a time.",
        "BMS Service Missing means ChargeScreen can see the battery but did not find the expected service.",
        "If that happens, send a BLE capture with the exact battery model."
      ]
    },
    {
      slug: "fogstar-drift",
      name: "Fogstar Drift Battery",
      type: "Lithium battery",
      status: "Supported",
      key: "No key normally needed",
      shows: ["State of charge", "Voltage", "Current", "Power", "Capacity"],
      setup: [
        "Add the Fogstar Drift screen in ChargeScreen setup.",
        "Fully close the Fogstar app on your phone.",
        "Turn phone Bluetooth off for a minute if the battery will not connect.",
        "Restart ChargeScreen and let it connect before opening the phone app again."
      ],
      tips: [
        "The Drift BMS can usually connect to only one device at a time.",
        "Authentication timeout normally points to another device still holding the BMS connection.",
        "On newer firmware, holding the ChargeScreen display can release the connection so the phone app can be used."
      ]
    },
    {
      slug: "roamer-battery",
      name: "Roamer Bluetooth Battery",
      type: "Lithium battery",
      status: "Supported",
      key: "No key normally needed",
      shows: ["State of charge", "Voltage", "Current", "Power", "Capacity"],
      setup: [
        "Add the Roamer battery screen in ChargeScreen setup.",
        "Close the Roamer app before testing ChargeScreen.",
        "Save settings and let ChargeScreen search.",
        "If it fails, restart ChargeScreen while the phone app remains closed."
      ],
      tips: [
        "Roamer compatibility has improved across Beta 8 and Beta 9.",
        "If readings are wrong or missing, send a BLE capture and a screenshot of the Roamer app readings.",
        "Keep the first test close to the battery."
      ]
    },
    {
      slug: "abc-bms-family",
      name: "ABC-BMS family",
      type: "Lithium battery",
      status: "New support",
      key: "No key normally needed",
      shows: ["State of charge", "Voltage", "Current", "Power", "Capacity"],
      setup: [
        "Add the ABC-BMS battery screen in ChargeScreen setup.",
        "Close any phone app connected to the BMS.",
        "Save settings and let ChargeScreen search.",
        "Send a BLE capture if your app variant is not recognised."
      ],
      tips: [
        "This family includes compatible ABC, SOK, NB and Hoover app devices.",
        "Support is still being tested across different labels and app variants.",
        "A capture plus the app's live readings is the best way to confirm a new variant."
      ]
    },
    {
      slug: "daly-foxygen",
      name: "Daly / Foxygen Bluetooth Battery",
      type: "Lithium battery",
      status: "Experimental",
      key: "No key normally needed",
      shows: ["State of charge", "Voltage", "Current", "Power", "Capacity"],
      setup: [
        "Add the Daly battery screen in ChargeScreen setup.",
        "Close the battery phone app before testing.",
        "Save settings and let ChargeScreen connect.",
        "If the data looks wrong, send a BLE capture and the app readings."
      ],
      tips: [
        "Daly and Foxygen support is experimental.",
        "Different BMS firmware versions may behave differently.",
        "Do the first test near the battery with other apps disconnected."
      ]
    },
    {
      slug: "renogy-battery",
      name: "Renogy Bluetooth Battery",
      type: "Lithium battery",
      status: "Experimental",
      key: "No key normally needed",
      shows: ["State of charge", "Voltage", "Current", "Power", "Capacity"],
      setup: [
        "Add the Renogy battery screen in ChargeScreen setup.",
        "Close the Renogy app before testing.",
        "Save settings and let ChargeScreen search for the Renogy Bluetooth module.",
        "If it does not appear, send a BLE capture and the app readings."
      ],
      tips: [
        "BT-TH style Renogy devices are expected to be recognised by ChargeScreen.",
        "Support is experimental until it has been confirmed on more real batteries.",
        "If the later captures look empty, update to the newest firmware and make a fresh capture."
      ]
    },
    {
      slug: "mopeka-lippert",
      name: "Mopeka / Lippert Tank Sensor",
      type: "Tank sensor",
      status: "Supported",
      key: "No key needed",
      shows: ["Depth", "Percentage", "Litres remaining", "Sensor battery"],
      setup: [
        "Add the Mopeka tank screen in ChargeScreen setup.",
        "Choose whether to show raw centimetres, percentage or litres.",
        "Set the tank calibration values if you want percentage or litres.",
        "Save settings and wait for the sensor advert."
      ],
      tips: [
        "If a sensor worked on older firmware but not a newer build, update to the latest Beta 9 build.",
        "Mopeka devices broadcast rather than staying connected like a battery.",
        "If no reading appears, check the sensor battery and confirm it is awake."
      ]
    },
    {
      slug: "ruuvi-sensor",
      name: "Ruuvi Sensor",
      type: "Environment sensor",
      status: "New support",
      key: "No key needed",
      shows: ["Temperature", "Humidity", "Pressure"],
      setup: [
        "Add the Ruuvi screen in ChargeScreen setup.",
        "Choose the Ruuvi sensor from the detected Bluetooth devices.",
        "Save settings and wait for a fresh reading.",
        "Only temperature, humidity and pressure are shown."
      ],
      tips: [
        "Ruuvi support is new in Beta 9.",
        "The ChargeScreen page is intentionally limited to temperature, humidity and pressure for now.",
        "If several Ruuvi sensors are nearby, use the detected address or label to pick the correct one."
      ]
    }
  ],
  wishlist: [
    {
      name: "Portable power stations",
      status: "Research only",
      note: "Potentially a large market, but not a promised support target. ChargeScreen would be monitor-only, showing readings where available, and would not control AC output, charge limits or other power-station settings."
    },
    {
      name: "Diesel heater controller",
      status: "Possible future support",
      note: "Interesting if reliable BLE data is available. Captures would need the heater model and matching app readings."
    },
    {
      name: "More Renogy devices",
      status: "Testing",
      note: "Renogy support is being built from real captures, so additional models are useful."
    },
    {
      name: "More ABC-BMS app variants",
      status: "Testing",
      note: "ABC, SOK, NB and Hoover app devices can vary. More captures help confirm compatibility."
    }
  ]
};
