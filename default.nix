with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "polybar-git";
  src = ./.;
  buildInputs =
  [
    boost
    pkgconfig

    xlibs.libXft
    xlibs.libXau
    xlibs.libXdmcp

    cmake
    expat

    xorg.xcbproto
    xorg.xcbutil
    xorg.xcbutilwm
    xorg.xcbutilimage
    xorg.libpthreadstubs
    xorg.libxcb

    python2
  ];
}
