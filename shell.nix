let
  pkgs = import <nixpkgs> {};
in
pkgs.mkShell {
    name = "scripted";
    buildInputs = with pkgs.buildPackages; [
        just
        lazygit
        xmake
        lua54Packages.lua
    ];
    shellHook = ''
        echo ""
        echo "Lua Scripting through WASM"
        echo ""
    '';
}
