alias bw := build_wasm
alias cw := copy_wasm
alias bc := build_copy_wasm
alias du := dev_ui
alias bu := build_ui
alias ba := build_all

# List available options
default:
    just --list

# Build C/++ code into WASM/JS with xmake and Emscripten
build_wasm:
  pushd ./wasm && xmake f -p wasm && xmake

# Copy the built WASM/JS into the `public` folder
copy_wasm:
  cp ./wasm/build/wasm/wasm32/release/webbed.js ./ui/public/
  cp ./wasm/build/wasm/wasm32/release/webbed.wasm ./ui/public/

# Build the C/++ code to WASM/JS and copy to `public` folder
build_copy_wasm: build_wasm copy_wasm

# Run the Vite live-reloading development server
dev_ui:
  pushd ./ui && npm run dev

# Type-check and build the Vue code for distribution
build_ui:
  pushd ./ui && npm run build

# Build everything for distribution
build_all: build_copy_wasm build_ui

