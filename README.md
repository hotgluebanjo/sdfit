# `sdfit`

Match imaging systems from DSV datasets.

This is a static redo of an [old Python wrapper](https://gist.github.com/hotgluebanjo/7dc7ed6568249b430abd4a56788a4b77) around [ALGLIB](https://www.alglib.net/)'s superb [RBF implementation](https://www.alglib.net/interpolation/fastrbf.php). That version is now a part of [`camera_match`](https://github.com/ethan-ou/camera-match).

Along with RBF, `sdfit` parameterizes the neural network included in ALGLIB as `mlp`.

## Usage

```
sdfit -h
```

```
sdfit v0.2.0
Scattered data fitting for tristimulus lookup tables.
https://github.com/hotgluebanjo

USAGE: sdfit <source> <target> [OPTIONS]

EXAMPLES:
  sdfit alexa.csv print-film.csv -d ',' -o alexa_to_print_film.cube
  sdfit venice.txt alexa.txt -m rbf -p 6 -f spi -o venice_to_alexa.spi3d

INPUTS:
  <source>   Plaintext file containing source dataset
  <target>   Plaintext file containing target dataset

OPTIONS:
  -h   Help
  -m   Method to use [mlp | rbf]              default: mlp
  -o   Output path and name                   default: 'output.cube'
  -d   Dataset delimiter [' ' | ',' | <tab>]  default: ' ' (space)
  -p   LUT print precision                    default: 8
  -c   LUT cube size                          default: 33
  -f   LUT format [cube | spi]                default: cube
  -s   RBF basis size                         default: 5.0
  -l   RBF layers                             default: 5
  -z   RBF smoothing                          default: 0.0
  -L   MLP layers                             default: 5
  -r   MLP restarts                           default: 5
```
