ED25519VER="7fa6712ef5d581a6981ec2b08ee623314cd1d1c4"

# Download ed25519
[ ! -d ed25519 ] && \
  get_archive "https://github.com/orlp/ed25519/archive/$ED25519VER.zip" \
  "ed25519-$ED25519VER.zip" "9eafd6483fb95cbd5acaced55be662b8d5b3a79e7f1fb2b7faf834b08daf989f"
[ -d ed25519-$ED25519VER ] && mv ed25519-$ED25519VER ed25519
