const forward = document.getElementById("forward");
const left = document.getElementById("left");
const spray = document.getElementById("spray");
const right = document.getElementById("right");
const backward = document.getElementById("backward");
const btnArray = document.querySelectorAll("button");
const stream = document.getElementById("stream");
const camStatus = document.querySelector(".cam-status");

let gateway = "ws://main-robot.local/ws";
let websocket;
let isConnected = false;
let lastFrameTime = Date.now();

window.addEventListener("load", onLoad);

const watchdog = setInterval(() => {
  if (Date.now() - lastFrameTime > 5000) {
    setCamStatus(false);
    restartStream();
  }
}, 3000);

function startStream() {
  stream.src = "http://live-stream.local";
}

function setCamStatus(connected) {
  camStatus.textContent = connected
    ? "Camera: Connected"
    : "Camera: Disconnected";

  if (connected) {
    stream.classList.remove("disconnected");
    camStatus.classList.remove("disconnected");
    stream.classList.add("connected");
    camStatus.classList.add("connected");
  } else {
    stream.classList.remove("connected");
    camStatus.classList.remove("connected");
    stream.classList.add("disconnected");
    camStatus.classList.add("disconnected");
  }
}

let restarting = false;

function restartStream() {
  if (restarting) return;
  restarting = true;
  stream.src = "";

  setTimeout(() => {
    stream.src = "http://live-stream.local";
    restarting = false;
  }, 1500);
}

function initWebSocket() {
  console.log("Trying to open a WebSocket connection...");
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage; // <-- add this line
}

function onOpen(event) {
  console.log("Connection opened");
  isConnected = true;
  getReadings();
}

function onClose(event) {
  console.log("Connection closed");
  if (isConnected) {
    clearReadings();
    isConnected = false;
  }
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  const data = JSON.parse(event.data);
  const keys = Object.keys(data);

  keys.forEach((key) => {
    document.getElementById(key).innerHTML = data[key];
  });
}

function onLoad(event) {
  initWebSocket();
  startStream();
  stream.addEventListener("load", () => {
    lastFrameTime = Date.now();
    setCamStatus(true);
  });

  stream.addEventListener("error", () => {
    setCamStatus(false);
    restartStream();
  });
}

function clearReadings() {
  document.getElementById("battery").innerHTML = "...";
  document.getElementById("volume").innerHTML = "...";
  document.getElementById("speed").innerHTML = "...";
  document.getElementById("signal").innerHTML = "...";
}

function getReadings() {
  websocket.send("getReadings");
}

forward.addEventListener("mousedown", (event) => {
  websocket.send("forward");
  console.log(event);
});

left.addEventListener("mousedown", () => {
  websocket.send("left");
});

spray.addEventListener("mousedown", () => {
  websocket.send("spray");
});

right.addEventListener("mousedown", () => {
  websocket.send("right");
});

backward.addEventListener("mousedown", () => {
  websocket.send("backward");
});

btnArray.forEach((item) => {
  if (item.id !== "spray") {
    item.addEventListener("mouseup", (event) => {
      console.log(event);
      websocket.send("stop");
    });
  }
});

spray.addEventListener("mouseup", () => {
  websocket.send("stop-spray");
});

setInterval(() => {
  if (websocket.readyState === WebSocket.OPEN) {
    websocket.send("ping");
  }
}, 5000);
