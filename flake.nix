{
  description = "Flake shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    mynixpkgs = {
      url = "git+ssh://github.com/gen740/mynixpkgs";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    inputs@{ flake-parts, nixpkgs, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = nixpkgs.lib.platforms.all;

      perSystem =
        { pkgs, ... }:
        {
          devShells.default = pkgs.mkShell {
            packages = [ ];
          };

          packages.default = pkgs.stdenv.mkDerivation {
            name = "demo";
            src = ./.;
            buildInputs = [
              pkgs.libgpiod_1
            ];
            installPhase = ''
              mkdir -p $out/bin
              cp -r ./epd $out/bin/epd
            '';
          };
        };
    };
}
