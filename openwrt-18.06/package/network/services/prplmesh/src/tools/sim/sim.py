###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

'''Simulator for a Multi-AP network.

The basic simulator framework adds a time aspect to the network model from the devices module.
It simulates time, events and messages, and triggers algorithms.

Time is based on ticks. A tick is 100us, and it's converted to seconds in all human-readable output.
Events (see below) are defined to happen at a tick, and algorithms can register callbacks to be
called at certain times. The simulator just triggers the next event or callback and updates the
current tick to that event/callback's tick.

Events are defined by a simulation scenario. An event is something like a device that shuts down
or starts up, or a link that changes quality or becomes unavailable.

Messages are sent by a device either to a specific other device, or broadcast to all devices. The
simulator checks that there is a backhaul link path to the target device(s) and that the target
device(s) are not shut down. If OK, the algorithm on the target device is triggered with this
message. A fixed delay is added to the message for each hop in the path.
'''

from collections import defaultdict
from devices import Link, Message, Network, Device
import logging
from typing import Any, Callable, Optional, Union


class Tick(int):
    '''A unit of time measured in simulator ticks (100us).'''

    per_second = 10000  # 100us

    def __str__(self):
        return f'{self/Tick.per_second:.4f}s'

    @staticmethod
    def s(seconds: float) -> "Tick":
        '''Create a Tick object representing the given number of seconds.'''
        return Tick(int(Tick.per_second*seconds))


class Event:
    '''Representation of an event that happens at a specific tick.

    The simulator will consecutively trigger the events at their allotted tick.

    When an event is triggered, its callback is executed. The callback determines the event.

    Parameters
    ----------
    when : Union[Tick, int]
        Tick at which the event will trigger. If passed as an int, it's converted to Ticks.
    callback : Callable(when: Tick, event: Any) -> None
        Callback that is called when the event triggers. Its argument `when` is the tick at which it
        is triggered.
    description : Any, default "Event"
        A human-readable description of the event. This is used for logging. It is also passed to
        the callback, so it can be used to provide context for the event. Finally, it is also passed
        to the checker, so the checker can do something with that information.

    Attributes
    ----------
    when : Tick
        Tick at which the event will trigger.
    callback : Callable(when: Tick, event: Any) -> None
        Callback that is called when the event triggers. Its argument `when` is the tick at which it
        is triggered.
    description : Any, default "Event"
        A human-readable description of the event. This is used for logging. It is also passed to
        the callback, so it can be used to provide context for the event. Finally, it is also passed
        to the checker, so the checker can do something with that information.
    '''

    def __init__(self, when: Union[Tick, int], callback: Callable[[Tick, Any], None],
                 description: Any = "Event"):
        self.when = Tick(when)
        self.callback = callback
        self.description = description

    def __str__(self):
        return f'{self.description}@{self.when}'


class Algorithm:
    '''Representation of an algorithm that runs on a specific device.

    This is a base class, intended to be derived from for the actual algorithm implementation.

    All callbacks have arguments `when` and `device`. `when` is the tick at which the callback
    happens. It can be used to schedule timeouts. `device` is the device on which the algorithm
    runs. This makes it possible to reuse the same simulation object for different devices. The
    algorithm can observe the links of the device object, and it can set the backhaul link. It must
    not manipulate the other devices in the network.

    Parameters
    ----------
    name: str
        Human-readable name. Used for logging.
    simulation: Simulation
        The Simulation to which this algorithm belongs. Used for creating events.

    Attributes
    ----------
    name: str
        Human-readable name. Used for logging.
    simulation: Simulation
        The Simulation to which this algorithm belongs. Used for creating events.
    '''
    def __init__(self, name: str, simulation: 'Simulation'):
        self.name = name
        self.simulation = simulation

    def __str__(self):
        return self.name

    def start(self, when: Tick, device: Device):
        '''Initialise the algorithm.

        To be implemented by derived classes.

        This method is called when the device starts up - typically at tick 0, but also if a device
        is added or reboots during the simulation run.

        It allows the algorithm to install timeouts. It is also possible to send messages at
        startup.

        Default implementation does nothing.

        Parameters
        ----------
        when : Tick
            Time point at which the device starts up.
        device : Device
            The device on which the algorithm runs.
        '''
        pass

    def handle_message(self, when: Tick, device: Device, message: Message, src: Device,
                       dst: Optional[Device], src_link: Optional[Link]):
        '''Handle an incoming message.

        To be implemented by derived classes.

        This method is called when a message arrives on the device. Its implementation should use
        the message.msg_type to determine if this algorithm is supposed to process it.

        Default implementation does nothing.

        Parameters
        ----------
        when : Tick
            Time point at which the message arrives.
        device : Device
            The device on which the message arrives.
        src: Device
            The sending device
        dst: Optional[Device]
            The destination device. If None, it's a multicast message.
        src_link: Optional[Link]
            The link on which the message arrives. If None, it's a locally generated message.
        '''
        pass

    def set_start(self, when: Tick, device: Device) -> None:
        '''Add a startup event at the specified tick.'''
        self.simulation.add_event(when, lambda when, _: self.start(when, device),
                                  f'{device}_{self.name}_Startup')

    def set_timeout(self, when: Union[Tick, int], device: Device,
                    callback: Callable[[Tick, Device, Any], None], description: Any = None) -> None:
        '''Schedule a callback to be executed in the future.

        Parameters
        ----------
        when : Union[Tick, int]
            Tick at which the timeout will trigger.
        callback : Callable(when: Tick, device: Device, description: Any) -> None
            Callback that is called when the timeout triggers. Its first argument `when` is the tick
            at which it is triggered. Its second argument `device` is the Device object on which
            the algorithm runs. Its third argument is the description.
        description : str, default "Timeout"
            A human-readable description of the timeout. This is used for logging. It is also used
        '''
        assert device in self.simulation.network.devices, "Device not in simulated network"
        assert self in self.simulation.algorithms[device], "Algorithm not running on device"

        if description is None:
            description = f'{device}_{self.name}_{description}'

        self.simulation.add_event(when, lambda when, descr: callback(when, device, descr),
                                  description)


class Simulation:
    '''Representation of a simulation run.

    A simulation is a sequence of events that is executed on a network, combined with algorithms
    that are run on devices.

    Parameters
    ----------
    network: Network
        Network on which the simulation runs.

    Attributes
    ----------
    network: Network
        Network on which the simulation runs.
    events: [Event]
        Scheduled events. This list is updated whenever an event is added. Events are removed from
        it when they are triggered.
    now: Tick
        The current simulation tick.
    algorithms: {Device: [Algorithm]}
        Mapping of device to list of algorithms running on that device. Automatically updated when
        an algorithm is added to a device.
    '''

    def __init__(self, network: Network):
        self.network = network
        self.events = []
        self.now = Tick()
        self.algorithms = defaultdict(list)

    def add_event(self, when: Union[Tick, int], callback: Callable[[Tick, Network], None],
                  description: str = "Event") -> None:
        '''Add a future event to the simulation.

        Parameters
        ----------
        when : Union[Tick, int]
            Tick at which the event will trigger. If it is a negative number, it is that many ticks
            from now.
        callback : Callable(when: Tick, network: Network) -> None
            Callback that is called when the event triggers. Its first argument `when` is the tick
            at which it is triggered. Its second argument `network` is the network object on which
            the event is triggered. These two arguments make it easy to reuse the same callback
            instance for different purposes.
        description : str, default "Event"
            A human-readable description of the event. This is used purely for logging.
        '''
        if when < 0:
            when = self.now - when
        event = Event(when, callback, description)
        assert when > self.now, f"{event} scheduled in the past, now={self.now:.3f}"
        logging.debug(f'Add event {event}')
        self.events.append(event)

    def add_algorithm_to_device(self, algorithm: Algorithm, device: Device) -> None:
        '''Add an algorithm to a specific device.'''
        assert algorithm.simulation is self, "{algorithm} associated with different simulation"
        assert device in self.network.devices, f"{device} not in simulated network"
        self.algorithms[device].append(algorithm)
        algorithm.set_start(self.now + 1, device)

    def run(self, checker: Callable[[Tick, Network, Any], None]) -> None:
        '''Run the simulation.

        This triggers the events in the order of their ticks.

        After each trigger, the checker function is called. The checker is intended to evaluate the
        simulated algorithms.

        Parameters
        ----------
        checker: Callable(when: Tick, network: Network, last_event: Any) -> None
            After an event has processed, the checker is called. Its arguments are `when`, the
            current simulation tick, `network`, the Network that is simulated, and `last_event`,
            the description of the last event that was simulated.
        '''
        while self.events:
            # Note that Python sort is stable, so events with the same `when` will be kept in the
            # order they were added.
            self.events.sort(key=lambda event: event.when)
            event = self.events.pop(0)
            when = event.when
            assert when >= self.now
            logging.debug(f'{when}: {event}')
            self.now = when
            event.callback(when, event.description)
            checker(when, self.network, event.description)


def forward_msg(message: Message, src: Device, dst: Optional[Device],
                on_device: Device, src_link: Optional[Link], simulation: Simulation):
    '''Forward a message according to the bridge logic.

    Parameters
    ----------
    message: Message
        The message to send. May also be a tuple from which the message is constructed.

    src: Device
        The sending device

    dst: Optional[Device]
        The destination device. If None, it's a multicast message.

    on_device: Device
        The device on which the forwarding happens.

    src_link: Optional[Link]
        The link on which the message arrives. If None, it's a locally generated message.

    simulation: Simulation
        The simulation in which to model this. Is used for timing. Every hop takes one Tick.
    '''
    assert src in simulation.network.devices
    assert not dst or dst in simulation.network.devices
    assert on_device in simulation.network.devices
    assert not src_link or on_device in src_link.devices

    if src_link:
        if src_link not in on_device.bridged_links:
            logging.debug(f"Incoming message from {src_link} not in bridge of {on_device}.")
            return

        # Update forwarding db
        on_device.forwarding_db[src] = src_link

    if dst == on_device or dst is None:
        logging.debug(f"{message} arrives on {on_device}")
        for algorithm in simulation.algorithms[on_device]:
            algorithm.handle_message(simulation.now, on_device, message, src, dst, src_link)

    if dst == on_device:
        # No forwarding
        return

    dst_link = on_device.forwarding_db.get(dst)
    if dst_link:
        if dst_link not in on_device.bridged_links:
            logging.debug(f"Link to {dst} from {on_device} forwarding_db is no longer "
                          "in the bridge: {dst_link}")
            dst_link = None

    if dst is None or dst_link is None:
        # Forward to all neighbours but don't hairpin
        forward_links = list(on_device.bridged_links)
        if src_link:
            forward_links.remove(src_link)
    else:
        forward_links = [dst_link]

    # Helper function for forwarding event
    def fwd(when, event):
        (_, src, dst, next_hop, link, simulation) = event
        forward_msg(message, src, dst, next_hop, link, simulation)

    for link in forward_links:
        simulation.add_event(-1, fwd, ("Forward", src, dst, link.other(on_device), link,
                                       simulation))


def send_msg(message: Message, src: Device, dst: Optional[Device], simulation: Simulation):
    '''Send a message to destination device.

    Send a message across the bridges to the destination device. It uses bridge logic and flooding
    to reach the destination. There is no error if the destination is not reachable. One simulation
    tick is added for every hop traversed.

    Parameters
    ----------
    message: Message
        The message to send. May also be a tuple from which the message is constructed.

    src: Device
        The sending device

    dst: Optional[Device]
        The destination device. If None, it's a multicast message.

    simulation: Simulation
        The simulation in which to model this. Is used for timing. Every hop takes one Tick.
    '''
    return forward_msg(message, src, dst, src, None, simulation)
