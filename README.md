# Computer Graphics: Halftone image approximation

The task is to compare algorithms of approximation of halftone image using bi-level image. I've used PGM format images.

## Algorithms

1. [Thresholding](https://en.wikipedia.org/wiki/Thresholding_(image_processing)) <br />
    I've used <b>(maxgrey + 1)/2</b> threshold.
2. [Random dithering](https://en.wikipedia.org/wiki/Dither)
3. [Ordered dithering](https://en.wikipedia.org/wiki/Ordered_dithering) <br />
    You can set threshold map size. Default value is 8.
4. [Error diffusion](https://en.wikipedia.org/wiki/Error_diffusion)
    1. Error diffusion forward (add 100% of error to the next pixel)
    2. Error diffusion forward for even strings and back for odd strings
    3. Floyd–Steinberg dithering forward
       <table>
       <tr align="center">
          <td></td>
          <td><b>X</b></td>
          <td>7/16</td>
        </tr>
        <tr align="center">
          <td>3/16</td>
         <td>5/16</td>
         <td>1/16</td>
        </tr>
        </table>
    4. Floyd–Steinberg dithering forward for even strings and back for odd strings <br />
       <b>For even strings</b>
        <table>
       <tr align="center">
          <td></td>
          <td><b>X</b></td>
          <td>7/16</td>
        </tr>
        <tr align="center">
          <td>3/16</td>
         <td>5/16</td>
         <td>1/16</td>
        </tr>
        </table>
        <b>For odd strings</b>
        <table>
       <tr align="center">
          <td>7/16</td>
          <td><b>X</b></td>
          <td></td>
        </tr>
        <tr align="center">
          <td>1/16</td>
         <td>5/16</td>
         <td>3/16</td>
        </tr>
        </table>
        
## PGM format

Image should be in [PGM](http://paulbourke.net/dataformats/ppm/) format.

## Usage

```bash
$ gcc image_approximation.c -o image_approximation
$ ./image_approximation
Usage: ./image_approximation <orig_image_path> <new_image_path> <algo_number> [index_matrix_size]
Algos:
1	Thresholding
2	Random dithering
3 	Ordered dithering
4	Error diffusion forward
		error diffusion forward to string
5	Error diffusion forward back
		error diffusion forward for even strings and back for odd strings
6	Error diffusion Floyd forward
		Floyd–Steinberg error diffusion forward to string
7	Error diffusion Floyd forward back
		Floyd–Steinberg error diffusion forward for even strings and back for odd strings
```

## Examples

You can see results of image approximation with different algorithms in **images/** folder. I've got test picture from [here](http://userpages.umbc.edu/~rostamia/2003-09-math625/images.html).
```bash
$ ./image_approximation ./images/reese_witherspoon.pgm ./images/reese_ordered_dithering16.pgm 3 16
```
