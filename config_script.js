const wifiAddForm = document.querySelector("#add-form");
const wifiListContent = document.querySelector(".content");
const closeBtn = document.querySelector("#close");
const overlay = document.querySelector(".overlay");
const modalHeading = document.querySelector("#modal-heading");
const updateForm = document.querySelector("#update-form");
const upperLimit = document.querySelector("#upper-limit");
const lowerLimit = document.querySelector("#lower-limit");

let wifiCredentials = [];

wifiAddForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const ssidEl = wifiAddForm.querySelector("#ssid");
  const passEl = wifiAddForm.querySelector("#password");

  // Send it to the esp
  const payload = {
    ssid: ssidEl.value,
    pass: passEl.value,
  };

  try {
    const response = await fetch("http://main-robot.local/add-credential", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(payload),
    });

    if (!response.ok) {
      throw new Error(request.statusText);
    }
    const data = await response.json();
    console.log(data);
    renderItems(wifiListContent, data?.ssids);
    lowerLimit.innerHTML = data?.ssids.length;
    upperLimit.innerHTML = data["max-num"];
  } catch (err) {
    console.log(err);
  }

  ssidEl.value = "";
  passEl.value = "";
});

updateForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const newPassEl = updateForm.querySelector("#new-password");

  try {
    const response = await fetch("http://main-robot.local/edit-credential", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({ ssid: editSsid, pass: newPassEl }),
    });

    if (!response.ok) {
      throw new Error(request.statusText);
    }
    const data = await response.json();
    console.log(data);
    renderItems(wifiListContent, data?.ssids);
    lowerLimit.innerHTML = data?.ssids.length;
    upperLimit.innerHTML = data["max-num"];
  } catch (err) {
    console.log(err);
  }
  newPassEl.value = "";
});

window.addEventListener("load", async (event) => {
  try {
    const request = await fetch("http://main-robot.local/list-credentials");

    if (!request.ok) throw new Error(request.statusText);
    const data = await request.json();

    wifiCredentials = Array.from(data?.ssids);
    updateCount(wifiCredentials.length, data["max-num"]);
  } catch (err) {
    console.log(err);
  }

  renderItems(wifiListContent, wifiCredentials);
});

overlay.addEventListener("click", (e) => {
  if (
    e.target === overlay ||
    (e.target.hasAttribute("id") && e.target.getAttribute("id") == "close")
  )
    overlay.classList.remove("active");
});

var editSsid;

function renderItems(container, items) {
  container.innerHTML = "";

  if (items.length === 0) {
    container.innerHTML = `<p>No wifi credential stored</>`;
    return;
  }

  items.forEach((item) => {
    const wifiCred = createWiFiCredential(item);
    container.appendChild(wifiCred);
  });
}

function createWiFiCredential(ssid) {
  const div = document.createElement("div");
  div.classList.add("wifi-credential");

  const ssidContainer = document.createElement("div");
  div.classList.add("ssid-container");

  const p = document.createElement("p");
  p.classList.add("flex");

  const icon = document.createElement("img");
  icon.setAttribute("src", "icons/wifi-icon.png");

  const innerp = document.createElement("p");
  innerp.innerHTML = ssid;

  p.appendChild(icon);
  p.appendChild(innerp);

  ssidContainer.appendChild(p);

  const divButtons = document.createElement("div");
  divButtons.classList.add("cred-btns");

  const editBtn = document.createElement("button");
  editBtn.innerText = "Edit";
  editBtn.classList.add("edit");
  editBtn.addEventListener("click", () => {
    overlay.classList.add("active");
    modalHeading.innerText = `Update Password for SSID: ${ssid}`;
    editSsid = ssid;
  });

  const deleteBtn = document.createElement("button");
  deleteBtn.innerText = "Delete";
  deleteBtn.classList.add("delete");
  deleteBtn.addEventListener("click", async () => {
    try {
      const response = await fetch(
        "http://main-robot.local/delete-credential",
        {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
          },
          body: JSON.stringify({ ssid: ssid }),
        }
      );

      if (!response.ok) {
        throw new Error(request.statusText);
      }
      const data = await response.json();
      console.log(data);
      renderItems(wifiListContent, data?.ssids);
      lowerLimit.innerHTML = data?.ssids.length;
      upperLimit.innerHTML = data["max-num"];
    } catch (err) {
      console.log(err);
    }
  });

  divButtons.appendChild(editBtn);
  divButtons.appendChild(deleteBtn);
  div.appendChild(ssidContainer);
  div.appendChild(divButtons);
  return div;
}

function updateCount(lower, upper) {
  lowerLimit.innerHTML = lower;
  upperLimit.innerHTML = upper;
}
