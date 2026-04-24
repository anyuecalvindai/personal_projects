"""a neural network class needs 3 functions: 
1.initialisation: set structure of neural network and initial weights/learning rate etc
2.train: modifying link weights after being given a training example
3.query: give an answer from the output node outputs
technically 4, but after training we can export the weights into a csv file and use that as the input weights the next time we run it, then we can keep refining it every time we run it. 
"""



import matplotlib.pyplot as plt
import numpy
import scipy.special
#from typing_extensions import dataclass_transform
import csv #library to write variables to a csv file


# neural network class definition
class neuralNetwork:


    # initialise the neural network
    def __init__(self, inputnodes, hiddennodes, outputnodes, learningrate):
        # set number of nodes in each input, hidden, output layer
        self.inodes = inputnodes
        self.hnodes = hiddennodes
        self.onodes = outputnodes

        # link weight matrices, wih and who
        # weights inside the arrays are w_i_j, where link is from node i to node j in the next layer
        # w11 w21
        # w12 w22 etc 
        self.wih = numpy.random.normal(0.0, pow(self.inodes, -0.5), (self.hnodes, self.inodes))
        self.who = numpy.random.normal(0.0, pow(self.hnodes, -0.5), (self.onodes, self.hnodes))

        # learning rate
        self.lr = learningrate

        # activation function is the sigmoid function
        self.activation_function = lambda x: scipy.special.expit(x)
        self.inverse_activation_function = lambda x: scipy.special.logit(x)
        pass


    # train the neural network
    def train(self, inputs_list, targets_list):
        # convert inputs list to 2d array
        inputs = numpy.array(inputs_list, ndmin=2).T
        targets = numpy.array(targets_list, ndmin=2).T

        # calculate signals into hidden layer
        hidden_inputs = numpy.dot(self.wih, inputs)
        # calculate the signals emerging from hidden layer
        hidden_outputs = self.activation_function(hidden_inputs)

        # calculate signals into final output layer
        final_inputs = numpy.dot(self.who, hidden_outputs)
        # calculate the signals emerging from final output layer
        final_outputs = self.activation_function(final_inputs)

        # output layer error is the (target - actual)
        output_errors = targets - final_outputs
        # hidden layer error is the output_errors, split by weights, recombinedinhnodes
        hidden_errors = numpy.dot(self.who.T, output_errors) 

        # update the weights for the links between the hidden and output layers
        self.who += self.lr * numpy.dot((output_errors * final_outputs * (1.0 - final_outputs)), numpy.transpose(hidden_outputs))

        # update the weights for the links between the input and hidden layers
        self.wih += self.lr * numpy.dot((hidden_errors * hidden_outputs * (1.0 - hidden_outputs)), numpy.transpose(inputs))

        #export files to a csv. Probably could be placed somewhere where it is not iterated over many times but cant be asked to figure that out lol
        global ih_matrix_export
        global ho_matrix_export
        ih_matrix_export = self.wih
        ho_matrix_export = self.who

        pass


    # query the neural network for the output
    def query(self, inputs_list):
        # convert inputs list to 2d array
        inputs = numpy.array(inputs_list, ndmin=2).T

        # calculate output from input layer to hidden layer
        hidden_inputs = numpy.dot(self.wih, inputs)
        # calculate the outputs from hidden layer
        hidden_outputs = self.activation_function(hidden_inputs)

        # calculate signals into final output layer
        final_inputs = numpy.dot(self.who, hidden_outputs)
        # calculate the signals emerging from final output layer
        final_outputs = self.activation_function(final_inputs)

        return final_outputs
    # backquery the neural network
    # we'll use the same termnimology to each item, 
    # eg target are the values at the right of the network, albeit used as input
    # eg hidden_output is the signal to the right of the middle nodes

# number of input, hidden and output nodes and learning rate setting
input_nodes = input("please set the number of input nodes or specify which program to use: ")
if input_nodes == "mnist" or input_nodes == "MNIST":
    input_nodes = 784
    hidden_nodes = 500
    output_nodes = 10
    learning_rate = 0.01
else: 
    input_nodes = int(input_nodes)
    hidden_nodes = int(input("please set the number of hidden nodes: "))
    output_nodes = int(input("please set the number of output nodes: "))
    learning_rate = float(input("please set the learning rate: "))


#create instance of neural network
n = neuralNetwork(input_nodes,hidden_nodes,output_nodes, learning_rate)



# load the mnist training data CSV file into a list
training_data_file = open("mnist_train_10000.csv", 'r')
training_data_list = training_data_file.readlines()
training_data_file.close()
# train the neural network

# epochs is the number of times the training data set is used for training
#brute iteration to try and get it to work
epochs = 7

for e in range(epochs):
    # go through all records in the training data set
    for record in training_data_list:
        # split the record by the ',' commas
        all_values = record.split(',')
        for i in range(len(all_values)): #convert all values to floats so that the maths eventually works out
          all_values[i] = float(all_values[i])
        # scale and shift the inputs from 0-1 
        
        inputs = (numpy.asarray(all_values[1:]) / 255.0 * 0.99) + 0.01
        # create the target output values (all 0.01, except the desired label which is 0.99)

        targets = numpy.zeros(output_nodes) + 0.01
        # all_values[0] is the target label for this record
        targets[int(all_values[0])] = 0.99
        n.train(inputs, targets)
        ## create rotated variations        
        # rotated anticlockwise by 10 degrees
        inputs_plus10_img = scipy.ndimage.rotate(inputs.reshape(28,28), 10, cval=0.01, order=1, reshape=False)
        n.train(inputs_plus10_img.reshape(784), targets)
        # rotated clockwise by 10 degrees
        inputs_minus10_img = scipy.ndimage.rotate(inputs.reshape(28,28), -10, cval=0.01, order=1, reshape=False)
        n.train(inputs_minus10_img.reshape(784), targets)
        pass
    pass

test_data_file = open("mnist_test_100.csv", 'r')
test_data_list = test_data_file.readlines()
test_data_file.close()

scorecard = []

# go through all the records in the test data set
for record in test_data_list:
    # split the record by the ',' commas
    all_values = record.split(',')
    # correct answer is first value
    correct_label = int(all_values[0])
    for i in range(len(all_values)): #convert all values to floats so that the maths eventually works out
        all_values[i] = float(all_values[i])
    # scale and shift the inputs
    inputs = (numpy.asarray(all_values[1:]) / 255.0 * 0.99) + 0.01
    # query the network
    outputs = n.query(inputs)
    # the index of the highest value corresponds to the label
    label = numpy.argmax(outputs)
    # append correct or incorrect to list
    print("network's answer is: ", label)
    print("correct label =      ", correct_label)
    print("--------------------------------------------")
    if (label == correct_label):
        # network's answer matches correct answer, add 1 to scorecard
        scorecard.append(1)
    else:
        # network's answer doesn't match correct answer, add 0 to scorecard
        scorecard.append(0)
        pass

    pass

scorecard_array = numpy.asarray(scorecard)
print ("performance = ", float(scorecard_array.sum() / scorecard_array.size))

#show the image of the final data point using matplot lib just to prove im not capping
plt.imshow(inputs.reshape(28, 28), cmap='Greys', interpolation='None')
plt.show()
