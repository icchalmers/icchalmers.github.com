# DrawingMachine.py
# 
# This is the driver for a Gestalt based drawing machine.
# It was designed at FabLab@Strathclyde as part of FabAcademy2015
# Full details are on our class webpage:
#     http://fabacademy.org/archives/2015/eu/labs/glasgow_s/drawingMachine.html
#
# This machine is based on Nadya Peek's two stage virtual machine example.

# Two stage example Virtual Machine file
# moves get set in Main
# usb port needs to be set in initInterfaces
# Nadya Peek Dec 2014
# Modified Iain Chalmers 2015

#------IMPORTS-------
import csv
from pygestalt import nodes
from pygestalt import interfaces
from pygestalt import machines
from pygestalt import functions
from pygestalt.machines import elements
from pygestalt.machines import kinematics
from pygestalt.machines import state
from pygestalt.utilities import notice
from pygestalt.publish import rpc	#remote procedure call dispatcher
import time
import io


#------VIRTUAL MACHINE------
class virtualMachine(machines.virtualMachine):
	
	def initInterfaces(self):
		if self.providedInterface: self.fabnet = self.providedInterface		#providedInterface is defined in the virtualMachine class.
		else: self.fabnet = interfaces.gestaltInterface('FABNET', interfaces.serialInterface(baudRate = 115200, interfaceType = 'ftdi', portName = '/dev/ttyUSB0'))
		
	def initControllers(self):
		self.x1AxisNode = nodes.networkedGestaltNode('X1 Axis', self.fabnet, filename = '086-005a.py', persistence = self.persistence)
		self.x2AxisNode = nodes.networkedGestaltNode('X2 Axis', self.fabnet, filename = '086-005a.py', persistence = self.persistence)
		self.yAxisNode = nodes.networkedGestaltNode('Y Axis', self.fabnet, filename = '086-005a.py', persistence = self.persistence)

		self.xNode = nodes.compoundNode(self.x1AxisNode, self.x2AxisNode)
		self.xyNode = nodes.compoundNode(self.x1AxisNode, self.x2AxisNode, self.yAxisNode)

	def initCoordinates(self):
		self.position = state.coordinate(['mm', 'mm', 'mm'])
	
	def initKinematics(self):
		self.x1Axis = elements.elementChain.forward([elements.microstep.forward(4), elements.stepper.forward(1.8), elements.leadscrew.forward(8), elements.invert.forward(False)])
		self.x2Axis = elements.elementChain.forward([elements.microstep.forward(4), elements.stepper.forward(1.8), elements.leadscrew.forward(8), elements.invert.forward(False)])
		self.yAxis = elements.elementChain.forward([elements.microstep.forward(4), elements.stepper.forward(1.8), elements.leadscrew.forward(8), elements.invert.forward(False)])		
		
		self.stageKinematics = kinematics.direct(3)	#direct drive on all axes
	
	def initFunctions(self):
		self.move = functions.move(virtualMachine = self, virtualNode = self.xyNode, axes = [self.x1Axis, self.x2Axis, self.yAxis], kinematics = self.stageKinematics, machinePosition = self.position,planner = 'null')
		self.jog = functions.jog(self.move)	#an incremental wrapper for the move function
		pass
		
	def initLast(self):
		#self.machineControl.setMotorCurrents(aCurrent = 0.8, bCurrent = 0.8, cCurrent = 0.8)
		#self.xNode.setVelocityRequest(0)	#clear velocity on nodes. Eventually this will be put in the motion planner on initialization to match state.
		pass
	
	def publish(self):
		#self.publisher.addNodes(self.machineControl)
		pass
	
	def getPosition(self):
		return {'position':self.position.future()}
	
	def setPosition(self, position  = [None]):
		self.position.future.set(position)

	def setSpindleSpeed(self, speedFraction):
		#self.machineControl.pwmRequest(speedFraction)
		pass

#------IF RUN DIRECTLY FROM TERMINAL------
if __name__ == '__main__':
	# The persistence file remembers the node you set. It'll generate the first time you run the
	# file. If you are hooking up a new node, delete the previous persistence file.
	stages = virtualMachine(persistenceFile = "DrawingMachine.vmp")
	
	# This is a widget for setting the potentiometer to set the motor current limit on the nodes.
	# The A4982 has max 2A of current, running the widget will interactively help you set. 
	# stages.xyNode.setMotorCurrent(0.5)

	# This is for how fast the machine moves
	stages.xyNode.setVelocityRequest(10)	

	# Plot Neil...
	#plotFile = 'gershenfeld.csv'

	# Plot a minion
	plotFile = 'evil_minion.csv'

	# parse a CSV file of plot points
	moves = [[float(0),float(0)]]
	with open(plotFile, 'rb') as csvfile:
		coordinates = csv.reader(csvfile, delimiter=',')
		for point in coordinates:
			moves.append([float(point[0]),float(point[1])])

	# The first move was just a dummy float so remove it
	del moves[0]
	
	# This is a sloppy way to wait but oh well
	raw_input("Remove the pen and press enter")
	
	# Now move the machine to the first draw point
	stages.move([moves[0][0],moves[0][0],moves[0][1]], 0)
	status = stages.yAxisNode.spinStatusRequest()
	while status['stepsRemaining'] > 0:
		time.sleep(0.001)
		status = stages.yAxisNode.spinStatusRequest()
		
	# Wait for the user to insert the pen
	raw_input("Insert the pen and press enter")

	# Draw!
	for move in moves:
		# notice one X is sent to two axis
		stages.move([move[0],move[0],move[1]], 0)
		status = stages.yAxisNode.spinStatusRequest()
		# This checks to see if the move is done.
		while status['stepsRemaining'] > 0:
			time.sleep(0.001)
			status = stages.yAxisNode.spinStatusRequest()

	raw_input("Finished drawing! Remove the pen and press any key to continue")

	# Now move the machine back to 0,0
	stages.move([0,0,0], 0)
	status = stages.yAxisNode.spinStatusRequest()
	while status['stepsRemaining'] > 0:
		time.sleep(0.001)
		status = stages.yAxisNode.spinStatusRequest()

	print("Done!")

