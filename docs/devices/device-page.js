(() => {
  const support = window.CHARGESCREEN_SUPPORT || {};
  const slug = document.body.dataset.deviceSlug;
  const device = (support.devices || []).find((item) => item.slug === slug);
  const escapeHtml = (value) =>
    String(value)
      .replaceAll("&", "&amp;")
      .replaceAll("<", "&lt;")
      .replaceAll(">", "&gt;")
      .replaceAll('"', "&quot;")
      .replaceAll("'", "&#39;");

  const text = (id, value) => {
    const element = document.getElementById(id);
    if (element && value) element.textContent = value;
  };

  const list = (id, values) => {
    const element = document.getElementById(id);
    if (!element || !values) return;
    element.innerHTML = values.map((value) => `<li>${escapeHtml(value)}</li>`).join("");
  };

  if (!device) {
    text("device-title", "Device not found");
    text("device-summary", "This support page could not find a matching device.");
    return;
  }

  document.title = `${device.name} - ChargeScreen setup notes`;
  text("device-title", device.name);
  text("device-type", device.type);
  text("device-status", device.status);
  text("device-key", device.key);
  text("device-summary", `${device.name} is ${device.status.toLowerCase()} by ChargeScreen. ${device.key}.`);
  list("device-shows", device.shows);
  list("device-setup", device.setup);
  list("device-tips", device.tips);

  const form = document.querySelector("[data-tip-form]");
  const message = document.getElementById("tip-form-message");
  const deviceField = document.getElementById("tip-device");
  const submit = form?.querySelector("button[type='submit']");
  const fallbackEmail = support.submission?.fallbackEmail || "contact@chargescreen.co.uk";

  if (deviceField) deviceField.value = device.name;

  if (form && support.submission?.enabled && support.submission?.endpoint) {
    form.action = support.submission.endpoint;
    form.method = "post";
    if (message) message.textContent = "Submissions are sent for review before anything appears on the website.";
    return;
  }

  if (submit) submit.textContent = "Email this tip";
  if (message) {
    message.innerHTML = `Online tip submissions are not connected yet. This button opens an email to <a href="mailto:${fallbackEmail}">${fallbackEmail}</a>.`;
  }

  form?.addEventListener("submit", (event) => {
    event.preventDefault();
    const data = new FormData(form);
    const subject = `ChargeScreen device tip: ${device.name}`;
    const body = [
      `Device: ${device.name}`,
      `Name: ${data.get("name") || ""}`,
      `Email: ${data.get("email") || ""}`,
      "",
      "Tip:",
      data.get("tip") || "",
      "",
      "What worked:",
      data.get("worked") || ""
    ].join("\n");
    window.location.href = `mailto:${fallbackEmail}?subject=${encodeURIComponent(subject)}&body=${encodeURIComponent(body)}`;
  });
})();
