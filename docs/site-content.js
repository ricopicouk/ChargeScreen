(async () => {
  try {
    const response = await fetch("site-content.json", { cache: "no-store" });
    if (!response.ok) return;
    const content = await response.json();

    const setText = (id, value) => {
      const element = document.getElementById(id);
      if (element && value) element.textContent = value;
    };

    const setLink = (id, url, text) => {
      const element = document.getElementById(id);
      if (!element) return;
      if (url) element.href = url;
      if (text) element.textContent = text;
    };

    const version = content.firmware?.version;
    setText("current-build", version);
    setText("latest-version", version);
    setText("latest-summary", content.firmware?.summary);
    setText("table-version", version);
    setText("table-changes", content.firmware?.changes);
    setLink("latest-download", content.firmware?.download_url, "Download");
    setLink("table-download", content.firmware?.download_url, "Firmware");

    const generalEmail = content.contact?.general_email;
    setLink("general-email", generalEmail ? `mailto:${generalEmail}` : "", generalEmail);

    const bleEmail = content.contact?.ble_email;
    document.querySelectorAll(".ble-email").forEach((element) => {
      if (bleEmail) {
        element.href = `mailto:${bleEmail}`;
        element.textContent = bleEmail;
      }
    });

    setText("power-instructions", content.instructions?.power);
    setText("victron-intro", content.instructions?.victron_intro);
    setText("victron-first-step", content.instructions?.victron_first_step);
    setText("ble-intro", content.instructions?.ble_intro);
    setText("ble-privacy", content.instructions?.ble_privacy);
    setText("purchase-text", content.purchase?.text);
    setLink("purchase-link", content.purchase?.url, content.purchase?.button_text);
  } catch {
    // The HTML defaults remain usable if the content file cannot be loaded.
  }
})();
