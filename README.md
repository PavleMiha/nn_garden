# MLGarden

What I wanted to do with ML Garden, is to go from an empty slate to a relatively complex neural network, using a visual editor, no linear algebra, and nothing past high school math. You can play along by downloading a release and building your own networks while you read.

MLGarden was inspired by watching Karpathy's [spelled-out intro to neural networks](https://www.youtube.com/watch?v=VMj-3S1tku0) and wishing that I could play with the visual computation graphs directly!

![image](https://github.com/user-attachments/assets/e22ebfd3-8181-429b-9e46-03fa3b8063e9)

So what can you do in ML Garden? You can make computation graphs such as this one:

![image](https://github.com/user-attachments/assets/17505b32-df2d-4c2c-80ad-325c8ece86ab)

All this is, is a different way of expressing (5 + 3) * 2 = 16

Why is this useful? Well, for one, we don't need parenthesis, you just do your calculations from left to right. But there's another advantage: say that we want to change the three parameters, a, b and c, to make the result 10 instead of 16. One way we can do this is by moving each parameter, checking whether the result is going closer or further from 10, and then nudge it in the direction that makes it closer to 10.

![nudge](https://github.com/user-attachments/assets/eaae4a93-26df-4bac-ab3f-54cc1afbe26d)

This is hardly efficient, so how can we systematise this somehow? What we are doing is trying to figure out how the result will change if we change a, b, or c. Another way to put this is that we are figuring out the derivative of the result with respect to each of the parameters. This is where the computation graph comes in handy, through a process called *backpropagation*.

Let's look at a simple case, addition. When we nudge each of the source nodes, the output node moves the same amount. The derivative of the output node with respect to each parameter is 1.

![image](https://github.com/user-attachments/assets/148ec9ac-35c3-4dea-8ea9-0cd94dd74b57)

The derivatives are written out in orange.

Lets look at a slightly more complicated case, multiplication. If we nudge the parameters, the amount the result moves depends on the value of the *other* parameter. Easy to see that if b is 0, no matter how much we move a the result won't change. If b is -1 the result will change by the opposite amount we change b. If b is 2, the result will change by twice the amount we change a. We simply copy the value of the other parameter into the derivative of each of the parameters.

![image](https://github.com/user-attachments/assets/c3d287df-38f1-4bb7-9d72-ce3d7bc5d4eb)

We can do this for any operation with 2 inputs. You can actually check these on [Wolfram Alpha](https://www.wolframalpha.com/) for some of the more complicated ones, if you don't feel like figuring it out.

![image](https://github.com/user-attachments/assets/34ba41c6-9f12-4ca1-8c90-c40ddb27c0ee)

But what if it's a bigger graph, like our initial example? We first start from the result node and calculate the derivatives of its immediate children

![image](https://github.com/user-attachments/assets/fcb6db41-2c89-4051-8dc6-ee4c1c283100)

But what do we do about that "a+b" graph. Intuitively, if we change a or b, we know the "a+b" node will change by the same amount, and we know that if the "a+b" node changes, the result node will change by twice that amount. So if we change a, the result will change by twice the amount. Essentially we take the derivative, and we multiply it backwards by the derivative of each individual node. That's back propagation!

![image](https://github.com/user-attachments/assets/eff71ad8-69bd-4c06-97ea-fdb1209eb790)

And the benefit of this is that we can keep going, no matter how deep the graph is! We just take a node's derivative value, meaning the result's derivative with respect to that node, and then to get its source nodes derivative we use the two node rules we discussed above, and multiply them by that node's derivative value.

![image](https://github.com/user-attachments/assets/a008aafd-ef5f-4abe-a03b-aef81341cda9)

You can scale this up to literally billions of operations! But getting back to our example, how can we make the result 10? We could notice that the result is higher than 10, and therefore nudge all the parameters in the direction that makes the result smaller, until it reaches 10

![derivative_nudge](https://github.com/user-attachments/assets/553d174b-a4af-4e4f-9cc7-0aa54f58aa9a)

Can we systematise this for any result we might want? We could create an **error function** that quantifies how wrong we are. A good example here is to take the result of the graph, subtract the desired result (10) from it, and then take the square of that difference, so that value will always go up as the error goes up. Then we could take the derivative of this error, and nudge all the parameters to make the error go down. That's the basis of all deep learning! As long as you can define an error function, you can optimise a large computation graph to lower that error. Let's see what that looks like:

![image](https://github.com/user-attachments/assets/fe9fd006-9932-467e-a13f-e0d59683a16b)

Now the we just have to nudge all the parameters by an amount that's the inverse of the derivative. We could for example take the derivative and multiply it by 0.01, and add it to each parameter's value. The 0.01 would be the **learning rate**.

But how is this useful? Basically anything that you can calculate and you can define an error rate for, you can now iteratively improve! If your error rate is how bad you are at predicting the next character in a piece of text, lowering that error rate makes your computation graph better at predicting text! Let's start with something simpler, sorting dots into 2 categories:

![image](https://github.com/user-attachments/assets/faaa2873-92b4-42bd-a6bc-033116fd1235)

This is our dataset, each dot is a data point, and each point has an x coordinate, and a y coordinate,

## How to build

```
git submodule update --init --recursive
mkdir build
cd build
cmake ..
```
