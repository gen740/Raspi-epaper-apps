{
  stdenv,
  lib,
  ...
}:
stdenv.mkDerivation rec {
  name = "stb_image";
  srcs = [
    (builtins.fetchurl {
      url = "https://github.com/nothings/stb/raw/f58f558c120e9b32c217290b80bad1a0729fbb2c/stb_image.h";
      sha256 = "sha256:1cq089gygwzcbg4nm4cxh75fyv9nk27yrynvh91qnj29bpijyk2r";
    })
    (builtins.fetchurl {
      url = "https://github.com/nothings/stb/raw/f58f558c120e9b32c217290b80bad1a0729fbb2c/stb_image_write.h";
      sha256 = "sha256:01aaj6v3q19jiqdcywr4q7r3901ksahm8qxkzy54dx4wganz1mfb";
    })
    (builtins.fetchurl {
      url = "https://github.com/nothings/stb/raw/f58f558c120e9b32c217290b80bad1a0729fbb2c/stb_image_resize2.h";
      sha256 = "sha256:1pyl23vb9c0mbslv7417j68n3bmmlbz0vgamh7n4ri13shgbwpxg";
    })
  ];
  dontConfigure = true;
  dontBuild = true;
  dontUnpack = true;
  installPhase = ''
    install -d $out/include/stb
    for src in ${lib.concatStringsSep " " srcs}; do
      name=$(basename $src | sed 's/^[^-]*-//')  # プレフィックス削除
      cp $src $out/include/stb/$name
    done
  '';
}
