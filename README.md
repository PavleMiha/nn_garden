# MLGarden

What I wanted to do with ML Garden, is to go from an empty slate to a relatively complex neural network, using a visual editor, no linear algebra, and nothing past high school math.

MLGarden was inspired by watching Karpathy's [spelled-out intro to neural networks](https://www.youtube.com/watch?v=VMj-3S1tku0) and wishing that I could play with the visual computation graphs directly!

![image](https://github.com/user-attachments/assets/e22ebfd3-8181-429b-9e46-03fa3b8063e9)

So what can you do in ML Garden? You can make computation graphs such as this one:

![image](https://github.com/user-attachments/assets/17505b32-df2d-4c2c-80ad-325c8ece86ab)

All this is, is a different way of expressing (5 + 3) * 2 = 16

Why is this useful? Well, for one, we don't need parenthesis, you just do your calculations from left to right. But there's another advantage: say that we want to change the three parameters, a, b and c, to make the result 10 instead of 16. One way we can do this is by moving each parameter, checking whether the result is going closer or further from 10, and then nudge it in the direction that makes it closer to 10.

![nudge](https://github.com/user-attachments/assets/eaae4a93-26df-4bac-ab3f-54cc1afbe26d)

A WIP toy to play around with visual representations of computation graphs. Does somewhat fast automatic differentiation on them, if you right click a node and ask to do a backwards pass from that node. You can also save graphs, and collapse parts of your graph into re-usable "functions".

![screenshot of nn garden](./screenshot.png)

## How to build

```
git submodule update --init --recursive
mkdir build
cd build
cmake ..
```
