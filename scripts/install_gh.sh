#!/bin/bash -e

# See https://github.com/cli/cli/blob/trunk/docs/install_linux.md#debian-ubuntu-linux-raspberry-pi-os-apt
curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | sudo dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | sudo tee /etc/apt/sources.list.d/github-cli.list > /dev/null
sudo apt -qq update
sudo apt -qq install gh

echo "Go to https://github.com/settings/tokens/new to create a new token, and enter it below:"
read GH_TOKEN
echo "Enter your GitHub username:"
read GH_USERNAME

mkdir -p ~/.config/gh/
cat > ~/.config/gh/config.yml << EOM
hosts: 
    github.com:
        oauth_token: ${GH_TOKEN}
        user: ${GH_USERNAME}
EOM
