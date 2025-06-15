{
  stdenv,
  ...
}:
stdenv.mkDerivation {
  name = "bcm2835";
  src = builtins.fetchurl {
    url = "http://www.airspayce.com/mikem/bcm2835/bcm2835-1.75.tar.gz";
    sha256 = "sha256-4+6P0Ka0depxyjYvwlh/oayf0D7THChWVIN3Mek0lv0=";
  };
}
