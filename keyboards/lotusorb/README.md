# LotusOrb firmware builds

LotusOrb uses Vial-QMK. The left and right halves have separate keymaps and UF2 files.

## Build with GitHub Actions

1. Push changes to `keyboards/lotusorb/` or open a pull request affecting that directory.
2. Open the repository's **Actions** tab and select **Build LotusOrb firmware**.
3. Open the run for your branch and commit. Check that both build jobs succeed.
4. Download both artifacts:
   - `lotusorb-vial_left`: contains `lotusorb_vial_left.uf2`
   - `lotusorb-vial_right`: contains `lotusorb_vial_right.uf2`
5. Extract the ZIP files and flash the matching UF2 to each half.

Artifacts are retained for 30 days. Downloading artifacts requires signing in to GitHub.
The workflow does not flash hardware or publish a release.

After the workflow exists on the default branch, you can also select **Run workflow**
in Actions and choose the branch to build. Before it is merged, push a change to
the workflow or LotusOrb files on the feature branch to trigger a build.

The build container is pinned to the same image digest as the existing Vial CI,
and the repository's recorded submodule revisions are checked out recursively.

## Build locally

From a configured Vial-QMK checkout with its submodules initialized:

```sh
qmk compile -kb lotusorb -km vial_left
qmk compile -kb lotusorb -km vial_right
```

## Flash

Disconnect USB power before connecting or disconnecting the TRRS cable.
Hold the RP2040 Core-A boot switch while connecting USB, then copy the correct
UF2 to the bootloader drive. Repeat for the other half.

See the [build guide](https://zenn.dev/yubinagasaru/articles/00e4f8b8c75949)
for assembly and initial checks.
