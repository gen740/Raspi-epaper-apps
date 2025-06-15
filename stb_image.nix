{
  stdenv,
  lib,
  ...
}:
stdenv.mkDerivation {
  # https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h
  name = "stb_image";
  src = builtins.fetchurl {
    url = "https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h";
    sha256 = "sha256:1cq089gygwzcbg4nm4cxh75fyv9nk27yrynvh91qnj29bpijyk2r";
  };
  dontConfigure = true;
  dontBuild = true;
  dontUnpack = true;
  installPhase = ''
    mkdir -p $out/include/stb
    cp $src $out/include/stb/stb_image.h
  '';
}
