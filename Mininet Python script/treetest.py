#!/usr/bin/python

from mininet.net import Mininet
from mininet.node import Controller, RemoteController, OVSController
from mininet.node import CPULimitedHost, Host, Node
from mininet.node import OVSKernelSwitch, UserSwitch
from mininet.node import IVSSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.link import TCLink, Intf
from subprocess import call

def myNetwork():

    net = Mininet( topo=None,
                   build=False,
                   ipBase='10.0.0.0/8')

    info( '*** Adding controller\n' )
    c4=net.addController(name='c4',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=2004)

    c3=net.addController(name='c3',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=2003)

    c5=net.addController(name='c5',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=2005)

    c6=net.addController(name='c6',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=2006)

    c0=net.addController(name='c0',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=2000)

    c1=net.addController(name='c1',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=2001)

    c2=net.addController(name='c2',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=2002)

    info( '*** Add switches\n')
    s1 = net.addSwitch('s1', cls=OVSKernelSwitch)
    s5 = net.addSwitch('s5', cls=OVSKernelSwitch)
    s6 = net.addSwitch('s6', cls=OVSKernelSwitch)
    s7 = net.addSwitch('s7', cls=OVSKernelSwitch)
    s4 = net.addSwitch('s4', cls=OVSKernelSwitch)
    s3 = net.addSwitch('s3', cls=OVSKernelSwitch)
    s2 = net.addSwitch('s2', cls=OVSKernelSwitch)

    info( '*** Add hosts\n')
    h4 = net.addHost('h4', cls=Host, ip='10.0.0.4', defaultRoute=None)
    h2 = net.addHost('h2', cls=Host, ip='10.0.0.2', defaultRoute=None)
    h3 = net.addHost('h3', cls=Host, ip='10.0.0.3', defaultRoute=None)
    h5 = net.addHost('h5', cls=Host, ip='10.0.0.5', defaultRoute=None)
    h6 = net.addHost('h6', cls=Host, ip='10.0.0.6', defaultRoute=None)
    h7 = net.addHost('h7', cls=Host, ip='10.0.0.7', defaultRoute=None)
    h8 = net.addHost('h8', cls=Host, ip='10.0.0.8', defaultRoute=None)
    h1 = net.addHost('h1', cls=Host, ip='10.0.0.1', defaultRoute=None)

    info( '*** Add links\n')
    net.addLink(s2, s4)
    net.addLink(s2, s5)
    net.addLink(s3, s6)
    net.addLink(s3, s7)
    net.addLink(s6, h5)
    net.addLink(s6, h6)
    net.addLink(s5, h3)
    net.addLink(s5, h4)
    net.addLink(s4, h2)
    net.addLink(s4, h1)
    net.addLink(s7, h7)
    net.addLink(s7, h8)
    net.addLink(s1, s2)
    net.addLink(s1, s3)

    info( '*** Starting network\n')
    net.build()
    info( '*** Starting controllers\n')
    for controller in net.controllers:
        controller.start()

    info( '*** Starting switches\n')
    net.get('s1').start([c2])
    net.get('s5').start([c6])
    net.get('s6').start([c5])
    net.get('s7').start([c4])
    net.get('s4').start([c0])
    net.get('s3').start([c3])
    net.get('s2').start([c1])

    info( '*** Post configure switches and hosts\n')

    CLI(net)
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    myNetwork()

