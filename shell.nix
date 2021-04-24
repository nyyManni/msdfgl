with (import <nixpkgs> {});
mkShell {
  LD_LIBRARY_PATH="/run/opengl-driver/lib:/run/opengl-driver-32/lib";
  buildInputs = [
    mesa
    meson
    ninja
    pkg-config
    glfw3
    fontconfig
    glew
    xorg.libX11
    ccls
    clang-tools
    lldb
  ];
}
