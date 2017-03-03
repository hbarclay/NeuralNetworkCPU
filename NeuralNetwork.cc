#include "NeuralNetwork.h"
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <memory>
#include <random>
#include <chrono>

double NeuralNetwork::randomWeight()
{
	double number = (*distribution)(generator);
	return number;
}

double NeuralNetwork::randomBias()
{
	double number = (*distribution)(generator);
	return number;
}

void NeuralNetwork::Initialize()
{
	unsigned seed  = std::chrono::system_clock::now().time_since_epoch().count();
	generator = *(new std::default_random_engine(seed));
	distribution = new std::normal_distribution<double>(0.0, 0.5);
	

	//construct vector of neurons
	for(int i = 0; i < totalNeurons; i++ )
	{
		biases[i] = randomBias();
		auto neuron = std::unique_ptr<Neuron>(new Neuron());
		neuron->bias = randomBias();
		neurons.push_back(std::move(neuron));
	}

	// construct vector of connections
	// order of for loops is important, as the contiguous nature of 
	// std::vector will be used to pass parts to openBLAS

	for(double x : weights)
		x = randomWeight();

	for(int i = 0; i < numLayer1Neurons; i++)
	{
		for(int j = 0; j < numInputNeurons; j++)
		{
			auto connection = std::unique_ptr<Connection>(new Connection());
			connection->from = j;
			connection->to = numInputNeurons + i;
			connection->weight = randomWeight();
			connections.push_back(std::move(connection));
		
			weights[i*numInputNeurons + j] = randomWeight();
		}
	}


	for(int i = 0; i < numLayer2Neurons; i++)
	{
		for(int j = 0; j < numLayer1Neurons; j++)
		{
			auto connection = std::unique_ptr<Connection>(new Connection());
			connection->from = numInputNeurons + j;
			connection->to = numInputNeurons + numLayer1Neurons + i;
			connection->weight = randomWeight();
			connections.push_back(std::move(connection));

			weights[i*numLayer1Neurons + j] = randomWeight();
		}
	}

	for(int i = 0; i < numOutputNeurons; i++)
	{
		for(int j = 0; j < numLayer2Neurons; j++)
		{
			auto connection = std::unique_ptr<Connection>(new Connection());
			connection->from = numInputNeurons + numLayer1Neurons + j;
			connection->to = numInputNeurons + numLayer1Neurons + numLayer2Neurons + i;
			connection->weight = randomWeight();
			connections.push_back(std::move(connection));

			weights[i+numLayer2Neurons + j] = randomWeight();	
		}
	}	
}

double NeuralNetwork::getOutput(int id)
{
	//return outputs[i];
	Neuron* neuron = neurons[id].get();
	return neuron->output;
}

void NeuralNetwork::setNeuronOutput(int id, double scale)
{
	Neuron* neuron = neurons[id].get();
	neuron->output = scale;
	
	outputs[id] = scale;
}
/*
void NeuralNetwork::feedForward()
{
	// openBLAS implementation of feedForward

	// for(outputs[numInputNeurons:numInputNeurons+numLayer1Neurons-1])
	// for each layer one neuron...
	// output = sigmoid(outputs[0:numInputNeurons-1] . 
			connections(weights)[(i-numInputNeurons)*numInputNeurons : (i-numInputNeurons)*numInputNeurons+numInputNeurons-1] + output.bias)


	for(int i = numInputNeurons; i<numInputNeurons+numLayer1Neurons; i++) {
		double* out;
		cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans, 1, 1, numInputNeurons, 1.0, outputs, 1, weights, 1, 1.0, out, 1); 
		output[i] = sigmoid();
	}


	double test[numLayer1Neurons];
	memset(test, 0, numLayer1Neurons*sizeof(double));

	for(int i = 0; i<numLayer1Neurons; i++) {
		for(int j = 0; j < numInputNeurons; j++) {
			test[i] += outputs[j] * weights[i*numInputNeurons+j] + biases[i+numInputNeurons];
		}
		test[i] = sigmoid(test[i]);
	}

	for(int i = 0; i < numLayer1Neurons; i++) {
		if(test[i] != outputs[i+numInputNeurons]) {
			std::cout << "OpenBLAS didnt work!!!";
			throw std::runtime_error::runtime_error("aaaaa");
		}
	}

}

*/
void NeuralNetwork::feedForward()
{
		
	// TODO optimize
	int num = numInputNeurons * numLayer1Neurons;

	for(unsigned i = 0; i < num; i++){
		Connection* connection = connections[i].get();
		neurons[connection->to]->partialsum += 
			neurons[connection->from]->output * connection->weight;	
	}	

	for(unsigned i = numInputNeurons; i < numInputNeurons + numLayer1Neurons; i++){
		Neuron* neuron = neurons[i].get();
		neuron->output = sigmoid(neuron->partialsum + neuron->bias);
	}

	for(unsigned i = num; i < num + numLayer1Neurons * numLayer2Neurons; i++){
		Connection* connection = connections[i].get();
		neurons[connection->to]->partialsum += 
			neurons[connection->from]->output * connection->weight;	
	}
	
	for(unsigned i = numInputNeurons+ numLayer1Neurons; i<numInputNeurons+numLayer1Neurons+numLayer2Neurons; i++){
		Neuron* neuron = neurons[i].get();
		neuron->output = sigmoid(neuron->partialsum + neuron->bias);
	}
	
	for(unsigned i = num + numLayer1Neurons * numLayer2Neurons; i < totalConnections; i++){
		Connection* connection = connections[i].get();
		neurons[connection->to]->partialsum += 
			neurons[connection->from]->output * connection->weight;	
	}

	for(unsigned i = numInputNeurons+numLayer1Neurons+numLayer2Neurons; i < totalNeurons; i++){
		Neuron* neuron = neurons[i].get();
		neuron->output = sigmoid(neuron->partialsum + neuron->bias);
	}	
}

std::unique_ptr<NeuralNetwork> NeuralNetwork::Clone()
{
	std::unique_ptr<NeuralNetwork> clone(new NeuralNetwork);
	clone->Initialize();

	for(int i = 0; i < totalConnections; i++) {
		clone->weights[i] = this->weights[i];
	}

	for(int i = 0; i < totalNeurons; i++) {
		clone->biases[i] = this->biases[i];
	}

	
	for(int i = 0; i < totalConnections; i++){
		clone->connections[i]->weight = this->connections[i]->weight;
	}

	for(int i = 0; i < totalNeurons; i++){
		clone->neurons[i]->bias = this->neurons[i]->bias;
	}

	return std::move(clone);
}


std::unique_ptr<NeuralNetwork> NeuralNetwork::Crossover(NeuralNetwork* other)
{
	std::unique_ptr<NeuralNetwork> offspring(new NeuralNetwork());
	offspring->Initialize();


	for(int i = 0; i < totalConnections; i++){
		double num = 1.0 * rand() / RAND_MAX;
		if(num < 0.5){
			offspring->weights[i] = this->weights[i];	
		} else {
			offspring->weights[i] = other->weights[i];
		}
	}

	for(int i = 0; i < totalNeurons; i++){
		double num = 1.0 * rand() / RAND_MAX;
		if(num < 0.5){
			offspring->biases[i] = this->biases[i];
		} else {
			offspring->biases[i] = other->biases[i];
		}
	}

	for(int i = 0; i < totalConnections; i++){
		double num = 1.0 * rand() / RAND_MAX;
		if(num < 0.5){
			offspring->connections[i]->weight = this->connections[i]->weight;	
		} else {
			offspring->connections[i]->weight = other->connections[i]->weight;
		}
	}

	for(int i = 0; i < totalNeurons; i++){
		double num = 1.0 * rand() / RAND_MAX;
		if(num < 0.5){
			offspring->neurons[i]->bias = this->neurons[i]->bias;
		} else {
			offspring->neurons[i]->bias = other->neurons[i]->bias;
		}
	}

	return std::move(offspring);
}

double NeuralNetwork::sigmoid(double x)
{
	double a = (1.0 / (1.0 + exp(-x)));
//	std::cout << a << " ";
	return a;
}

void NeuralNetwork::Dump()
{
	std::cout << "Network: \n";
	for(int i = 0; i < totalConnections; i++){
		Connection *connection = connections[i].get();
		std::cout << connection->weight << "\n";
	}
}
