# MLGarden

MLGarden allows you to go from an empty slate to a relatively complex neural network, using a visual editor, no linear algebra, and nothing past high school math. You can play along by downloading a release and building your own networks while you read.

MLGarden was inspired by watching Karpathy's [spelled-out intro to neural networks](https://www.youtube.com/watch?v=VMj-3S1tku0) and wishing that I could play with the visual computation graphs directly!
<p align="center">
<img src="https://github.com/user-attachments/assets/e22ebfd3-8181-429b-9e46-03fa3b8063e9" width="700" >
</p>

So what can you do in ML Garden? You can make computation graphs such as this one:

<p align="center">
<img src="https://github.com/user-attachments/assets/17505b32-df2d-4c2c-80ad-325c8ece86ab" width="700" >
</p>

All this is, is a different way of expressing (5 + 3) * 2 = 16

Why is this useful? Well, for one, we don't need parenthesis, you just do your calculations from left to right. But there's another advantage: say that we want to change the three parameters, a, b and c, to make the result 10 instead of 16. One way we can do this is by moving each parameter, checking whether the result is going closer or further from 10, and then nudge it in the direction that makes it closer to 10.

<p align="center">
<img src="https://github.com/user-attachments/assets/eaae4a93-26df-4bac-ab3f-54cc1afbe26d" width="700" >
</p>

This is hardly efficient, so how can we systematise this somehow? What we are doing is trying to figure out how the result will change if we change a, b, or c. Another way to put this is that we are figuring out the derivative of the result with respect to each of the parameters. This is where the computation graph comes in handy, through a process called *backpropagation*.

Let's look at a simple case, addition. When we nudge each of the source nodes, the output node moves the same amount. The derivative of the output node with respect to each parameter is 1.

<p align="center">
<img src="https://github.com/user-attachments/assets/148ec9ac-35c3-4dea-8ea9-0cd94dd74b57" width="700" >
</p>

The derivatives are written out in orange.

Lets look at a slightly more complicated case, multiplication. If we nudge the parameters, the amount the result moves depends on the value of the *other* parameter. Easy to see that if b is 0, no matter how much we move a the result won't change. If b is -1 the result will change by the opposite amount we change b. If b is 2, the result will change by twice the amount we change a. We simply copy the value of the other parameter into the derivative of each of the parameters.

<p align="center">
<img src="https://github.com/user-attachments/assets/c3d287df-38f1-4bb7-9d72-ce3d7bc5d4eb" width="700" >
</p>

We can do this for any operation with 2 inputs. You can actually check these on [Wolfram Alpha](https://www.wolframalpha.com/) for some of the more complicated ones, if you don't feel like figuring it out.

<p align="center">
<img src="https://github.com/user-attachments/assets/34ba41c6-9f12-4ca1-8c90-c40ddb27c0ee" width="700" >
</p>

But what if it's a bigger graph, like our initial example? We first start from the result node and calculate the derivatives of its immediate children

<p align="center">
<img src="https://github.com/user-attachments/assets/fcb6db41-2c89-4051-8dc6-ee4c1c283100" width="700" >
</p>

But what do we do about that "a+b" graph. Intuitively, if we change a or b, we know the "a+b" node will change by the same amount, and we know that if the "a+b" node changes, the result node will change by twice that amount. So if we change a, the result will change by twice the amount. Essentially we take the derivative, and we multiply it backwards by the derivative of each individual node. That's back propagation!

<p align="center">
<img src="https://github.com/user-attachments/assets/eff71ad8-69bd-4c06-97ea-fdb1209eb790" width="700" >
</p>

And the benefit of this is that we can keep going, no matter how deep the graph is! We just take a node's derivative value, meaning the result's derivative with respect to that node, and then to get its source nodes derivative we use the two node rules we discussed above, and multiply them by that node's derivative value.

<p align="center">
<img src="https://github.com/user-attachments/assets/a008aafd-ef5f-4abe-a03b-aef81341cda9" width="700" >
</p>

You can scale this up to literally billions of operations! But getting back to our example, how can we make the result 10? We could notice that the result is higher than 10, and therefore nudge all the parameters in the direction that makes the result smaller, until it reaches 10

<p align="center">
<img src="https://github.com/user-attachments/assets/553d174b-a4af-4e4f-9cc7-0aa54f58aa9a" width="700" >
</p>

Can we systematise this for any result we might want? We could create an **error function** that quantifies how wrong we are. A good example here is to take the result of the graph, subtract the desired result (10) from it, and then take the square of that difference, so that value will always go up as the error goes up. Then we could take the derivative of this error, and nudge all the parameters to make the error go down. That's the basis of all deep learning! As long as you can define an error function, you can optimise a large computation graph to lower that error. Let's see what that looks like:

<p align="center">
<img src="https://github.com/user-attachments/assets/3195aa37-b2fd-4227-acf0-42293c1a7b95" width="700" >
</p>


Now that we have our error funtion, we just have to nudge all the parameters by an amount that's the inverse of the derivative. We could for example take the derivative and multiply it by 0.01, and add it to each parameter's value. The 0.01 would be the **learning rate**.

But how is this useful? Basically anything that you can calculate and you can define an error rate for, you can now iteratively improve! If your error rate is how bad you are at predicting the next character in a piece of text, lowering that error rate makes your computation graph better at predicting text, and it turns out that if get good enough at predicting the next token of text Let's start with something simpler, sorting dots into 2 categories:

<p align="center">
<img src="https://github.com/user-attachments/assets/12bb658e-cdea-4d7a-b266-cf7b0cc367e7" width="700" >
</p>

This is our dataset, each dot is a data point, and each point has an x coordinate, a y coordinate, and a label. Our artificial task is to predict, for a given set of coordinates, if the label should be 1- (blue in the image) or 1 (red). So let's define our error function first:

<p align="center">
<img src="https://github.com/user-attachments/assets/e406ba08-2655-41b0-ad87-2c92e966d1f3" width="700" >
</p>

So we will make a prediction, somehow, and then subtract from that prediction the result we know is right (the label), and then square that difference (so that the error is always positive, and so that decreasing the error is always good). Note that there are actually other was of defining the error function, they mostly all work as long as you get a more useful result when the error goes down.

How do we predict this value? Back propagation is pretty forgiving, in this case as long as we have a function that has the ability to describe this dataset, it'll find the right free parameters for it. Now it would be nicer if we didn't have to go in and manually poke the parameters around, so there's a Training window that'll do this process for you, you simply press play and it'll nudge every free paramater by it's derivative to the backprop node (usually the error), go to the next data point, and do it again. As an additional nicety it'll color in the dataset window based on the result node - checking what the result would be for every point in the graph window. Let's try a potential function, just taking the x coordinate of each point, multiplying it by some value, adding some other value to it, and then using [tanh](https://reference.wolfram.com/language/ref/Tanh.html#:~:text=Tanh%20is%20the%20hyperbolic%20tangent,of%20the%20natural%20logarithm%20Log.) to slam it to either -1 or 1.

<p align="center">
<img src="https://github.com/user-attachments/assets/918c9147-a3b7-4804-9be6-5532cf5258d9" width="700" >
</p>

Close, but it seems like the model can only really predict vertical lines - this makes sense, it only knows about each point's x coordinate, it can only represent . Let's instead add in the y coordinate, also multiplied by a parameter, to the x coordinate, and then run the training again.

<p align="center">
<img src="https://github.com/user-attachments/assets/90a0845d-2c17-4229-bccb-1c944c60b0f7" width="700" >
</p>

We did it! Although, we probably could have figured out an equation for this dataset by hand, let's try some tougher ones.

<p align="center">
<img src="https://github.com/user-attachments/assets/0710da57-7f83-416c-aedf-ab1637777017" width="700" >
</p>

A donut. Let's try it with out previous model

After we have this function defined, we can go point by point in the dataset and adjust our free parameters to make the error smaller and smaller and smaller. Let's try it.

<p align="center">
<img src="https://github.com/user-attachments/assets/7ee400fd-25cd-4c59-b0d6-5fd5d2b3bbf2)" width="700" >
</p>

Very quickly the model seems to give up, and simply label everything as halfway in between both labels - this makes sense. The model can only express datasets that are split by a line that passes through 0, 0. That's its *inductive bias*. So in this case, what reduces the error is to simply predict 0 for whichever point you pass in. 


## How to build

```
git submodule update --init --recursive
mkdir build
cd build
cmake ..
```
