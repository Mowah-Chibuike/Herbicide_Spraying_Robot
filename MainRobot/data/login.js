const message_p = document.getElementById("message");
const form = document.querySelector("form");

form?.addEventListener("submit", async (event) => {
  event.preventDefault();

  const form_data = new FormData(form);

  try {
    const response = await fetch("/login", {
      method: "POST",
      body: form_data,
    });

    if (response.ok) {
      window.location.href = "/";
    } else {
      const errorMessage = await response.text();
      throw new Error(
        `HTTP error! status: ${response.status}, message: ${errorMessage}`
      );
    }
  } catch (err) {
    console.log(err);
    message_p.textContent = "Username and Password pair not found!";
  }
});
