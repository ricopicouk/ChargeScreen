(function () {
  const buyPassword = "potato";
  const unlockedKey = "chargescreen-buy-unlocked";
  const scriptURL = "https://sdks.shopifycdn.com/buy-button/latest/buy-button-storefront.min.js";
  const products = [
    { id: "15892692238668", nodeId: "product-component-1783868010061", hideDescription: true },
    { id: "15892743815500", nodeId: "product-component-1783868059346" },
  ];
  let shopifyStarted = false;

  const buttonStyles = {
    "background-color": "#38bf72",
    "border": "2px solid #172126",
    "border-radius": "0",
    "color": "#172126",
    "font-family": "Inter, Helvetica Neue, sans-serif",
    "font-size": "16px",
    "font-weight": "900",
    "padding": "12px 18px",
    ":hover": {
      "background-color": "#32ac66",
      "color": "#172126",
    },
    ":focus": {
      "background-color": "#32ac66",
      "color": "#172126",
    },
  };

  const productOptions = {
    product: {
      styles: {
        product: {
          "@media (min-width: 601px)": {
            "max-width": "100%",
            "margin-left": "0",
            "margin-bottom": "0",
          },
          "border-bottom": "1px solid #d9e1df",
          "font-family": "Inter, Helvetica Neue, sans-serif",
          "padding-bottom": "22px",
          "text-align": "left",
        },
        title: {
          "color": "#172126",
          "font-family": "Inter, Helvetica Neue, sans-serif",
          "font-size": "22px",
          "font-weight": "900",
        },
        price: {
          "color": "#172126",
          "font-family": "Inter, Helvetica Neue, sans-serif",
          "font-size": "18px",
          "font-weight": "900",
        },
        description: {
          "color": "#60727a",
          "font-family": "Inter, Helvetica Neue, sans-serif",
          "font-size": "15px",
        },
        button: buttonStyles,
      },
      contents: {
        img: false,
        imgWithCarousel: false,
        description: true,
      },
      layout: "vertical",
      width: "100%",
      text: {
        button: "Add to cart",
      },
    },
    cart: {
      styles: {
        button: buttonStyles,
      },
      text: {
        total: "Subtotal",
        button: "Checkout",
      },
    },
    toggle: {
      styles: {
        toggle: {
          "background-color": "#38bf72",
          "border": "2px solid #172126",
          "border-radius": "0",
          ":hover": {
            "background-color": "#32ac66",
          },
          ":focus": {
            "background-color": "#32ac66",
          },
        },
        count: {
          "color": "#172126",
        },
        iconPath: {
          "fill": "#172126",
        },
      },
    },
    option: {},
  };

  function showProducts() {
    const gate = document.getElementById("buy-gate");
    const productsWrap = document.getElementById("shopify-products");

    if (gate) gate.hidden = true;
    if (productsWrap) productsWrap.hidden = false;
  }

  function unlockBuySection() {
    sessionStorage.setItem(unlockedKey, "true");
    showProducts();
    startShopify();
  }

  function setupBuyGate() {
    const gate = document.getElementById("buy-gate");
    const input = document.getElementById("buy-password");
    const message = document.getElementById("buy-gate-message");

    if (!gate || !input) return;

    gate.addEventListener("submit", (event) => {
      event.preventDefault();

      if (input.value === buyPassword) {
        unlockBuySection();
        return;
      }

      input.value = "";
      input.focus();
      if (message) {
        message.textContent = "Incorrect password.";
        message.classList.add("error");
      }
    });
  }

  function loadScript() {
    const script = document.createElement("script");
    script.async = true;
    script.src = scriptURL;
    script.onload = initShopifyBuy;
    (document.head || document.body).appendChild(script);
  }

  function initShopifyBuy() {
    const client = ShopifyBuy.buildClient({
      domain: "yfwtxb-1s.myshopify.com",
      storefrontAccessToken: "65209906211333b074cb164d7a7a42ed",
    });

    ShopifyBuy.UI.onReady(client).then((ui) => {
      products.forEach((product) => {
        const node = document.getElementById(product.nodeId);
        if (!node) return;

        const options = product.hideDescription
          ? {
              ...productOptions,
              product: {
                ...productOptions.product,
                contents: {
                  ...productOptions.product.contents,
                  description: false,
                },
              },
            }
          : productOptions;

        ui.createComponent("product", {
          id: product.id,
          node,
          moneyFormat: "%C2%A3%7B%7Bamount%7D%7D",
          options,
        });
      });
    });
  }

  function startShopify() {
    if (shopifyStarted) return;
    shopifyStarted = true;

    if (window.ShopifyBuy) {
      if (window.ShopifyBuy.UI) {
        initShopifyBuy();
      } else {
        loadScript();
      }
    } else {
      loadScript();
    }
  }

  setupBuyGate();

  if (sessionStorage.getItem(unlockedKey) === "true") {
    showProducts();
    startShopify();
  }
})();
