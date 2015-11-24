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
    c2=net.addController(name='c2',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=2002)

    c3=net.addController(name='c3',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=2003)

    c1=net.addController(name='c1',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=2001)

    c0=net.addController(name='c0',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=2000)

    info( '*** Add switches\n')
    s4 = net.addSwitch('s4', cls=OVSKernelSwitch)
    s1 = net.addSwitch('s1', cls=OVSKernelSwitch)
    s2 = net.addSwitch('s2', cls=OVSKernelSwitch)
    s3 = net.addSwitch('s3', cls=OVSKernelSwitch)

    info( '*** Add hosts\n')
    h2 = net.addHost('h2', cls=Host, ip='10.0.0.2', defaultRoute=None)
    h1 = net.addHost('h1', cls=Host, ip='10.0.0.1', defaultRoute=None)

    info( '*** Add links\n')
    h1s1 = {'bw':1000,'delay':'5ms'}
    net.addLink(h1, s1, cls=TCLink , **h1s1)
    s1s2 = {'bw':1000,'delay':'5ms'}
    net.addLink(s1, s2, cls=TCLink , **s1s2)
    s2s3 = {'bw':1000,'delay':'5ms'}
    net.addLink(s2, s3, cls=TCLink , **s2s3)
    s3s4 = {'bw':1000,'delay':'5ms'}
    net.addLink(s3, s4, cls=TCLink , **s3s4)
    s4s1 = {'bw':1000,'delay':'5ms'}
    net.addLink(s4, s1, cls=TCLink , **s4s1)
    s1s3 = {'bw':1000,'delay':'5ms'}
    net.addLink(s1, s3, cls=TCLink , **s1s3)
    s4s2 = {'bw':1000,'delay':'5ms'}
    net.addLink(s4, s2, cls=TCLink , **s4s2)
    h2s3 = {'bw':1000,'delay':'5ms'}
    net.addLink(h2, s3, cls=TCLink , **h2s3)

    info( '*** Starting network\n')
    net.build()
    info( '*** Starting controllers\n')
    for controller in net.controllers:
        controller.start()

    info( '*** Starting switches\n')
    net.get('s4').start([c3])
    net.get('s1').start([c0])
    net.get('s2').start([c1])
    net.get('s3').start([c2])

    info( '*** Post configure switches and hosts\n')

    CLI(net)
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    myNetwork()

