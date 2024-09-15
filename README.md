# MLGarden

What I wanted to do with ML Garden, is to go from an empty slate to a relatively complex neural network, using a visual editor, no linear algebra, and nothing past high school math.

MLGarden was inspired by watching Karpathy's [spelled-out intro to neural networks](https://www.youtube.com/watch?v=VMj-3S1tku0) and wishing that I could play with the visual computation graphs directly!

![image](https://github.com/user-attachments/assets/e22ebfd3-8181-429b-9e46-03fa3b8063e9)

So what can you do in ML Garden? You can make computation graphs such as this one:



A WIP toy to play around with visual representations of computation graphs. Does somewhat fast automatic differentiation on them, if you right click a node and ask to do a backwards pass from that node. You can also save graphs, and collapse parts of your graph into re-usable "functions".

![screenshot of nn garden](./screenshot.png)

## How to build

```
git submodule update --init --recursive
mkdir build
cd build
cmake ..
```
