<img align="right" title="Automaton Logo" width="128" height="128" src="media/automaton-avatar-64x64.svg">

# Automaton

## Build Status

<img src="https://ci.appveyor.com/api/projects/status/5euy83253gjqdasg/branch/master?svg=true">

Table of Contents
=================

  * [License](#license)
  * [Building Automaton](#building-automaton)
  * [Getting Started](#getting-started)

## License

#### Everything in this repo is licensed under the MIT License.

Check the [LICENSE](LICENSE) file for more details.

#### 🚨  HIGHLY EXPERIMENTAL! USE AT YOUR OWN RISK!

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

## Contact

Telegram: https://t.me/automaton_network

## Building Automaton

### Prerequisites

* Git
* Bazel

### External Third Party Libraries

First you need to run a script to download and build some required third party libraries.

**Linux & Mac OS**

```
./download_and_build_third_party_libs.sh
```

**Windows**

1. Start [Developer Command Prompt for Visual Studio 2017](https://docs.microsoft.com/en-us/dotnet/framework/tools/developer-command-prompt-for-vs)
2. In the started command prompt type in ``powershell``
3. Move to the project's directory ``cd "<path_to_project>"``
4. Run ``.\download_and_build_third_party_libs_windows.ps1``
5. Build and run with Bazel

### Bazel Build & Run

Once you have bazel you can run the following to build and run Automaton Core

**Linux & Mac OS**
```
./build-and-run-automaton-core.sh
```
**Windows**
```
.\build-and-run-automaton-core.ps1
```

## Getting Started

*COMING SOON!*

We're getting our step-by-step tutorials ready.
