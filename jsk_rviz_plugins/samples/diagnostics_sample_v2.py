#!/usr/bin/env python

# -*- coding: utf-8 -*-

import roslib
roslib.load_manifest('diagnostic_updater')
import rospy
import diagnostic_updater
import diagnostic_msgs
import std_msgs


time_to_launch = 0


def dummy_diagnostic(stat):
    if time_to_launch < 10:
        # summary for formatted text.
        stat.summary(diagnostic_msgs.msg.DiagnosticStatus.ERROR,
            "Buckle your seat belt. Launch in %f seconds!" % time_to_launch)
    else:
        # summary for unformatted text. It's just the same ;)
        stat.summary(diagnostic_msgs.msg.DiagnosticStatus.OK,
            "Launch is in a long time. Have a soda.")

    # add is used to append key-value pairs.
    # Again, as oppose to the C++ API, there is no addf function. The second
    # argument is always converted to a str using the str() function.
    stat.add("Diagnostic Name", "dummy")
    # add transparently handles conversion to string (using str()).
    stat.add("Time to Launch", time_to_launch)
    # add allows arbitrary printf style formatting.
    stat.add("Geeky thing to say", "The square of the time to launch %f is %f" % \
        (time_to_launch, time_to_launch * time_to_launch) )

    # As opposed to the C++ diagnostic function which modifies its argument,
    # the python version must return the modified message.
    return stat


class DummyClass:
    def produce_diagnostics(self, stat):
        stat.summary(diagnostic_msgs.msg.DiagnosticStatus.WARN, "This is a silly updater.")
        stat.add("Stupidicity of this updater", 1000.)
        return stat


class DummyTask(diagnostic_updater.DiagnosticTask):
    def __init__(self):
        diagnostic_updater.DiagnosticTask.__init__(self,
            "Updater Derived from DiagnosticTask")

    def run(self, stat):
        stat.summary(diagnostic_msgs.msg.DiagnosticStatus.WARN,
            "This is another silly updater.")
        stat.add("Stupidicity of this updater", 2000.)
        return stat


def check_lower_bound(stat):
    if time_to_launch > 5:
        stat.summary(diagnostic_msgs.msg.DiagnosticStatus.OK, "Lower-bound OK")
    else:
        stat.summary(diagnostic_msgs.msg.DiagnosticStatus.ERROR, "Too low")
    stat.add("Low-Side Margin", time_to_launch - 5)
    return stat


def check_upper_bound(stat):
    if time_to_launch < 10:
        stat.summary(diagnostic_msgs.msg.DiagnosticStatus.OK, "Upper-bound OK")
    else:
        stat.summary(diagnostic_msgs.msg.DiagnosticStatus.WARN, "Too high")
    stat.add("Top-Side Margin", 10 - time_to_launch)
    return stat


if __name__=='__main__':
    rospy.init_node("diagnostic_updater_example")

    updater = diagnostic_updater.Updater()

    updater.setHardwareID("none")
    # Or...
    updater.setHardwareID("Device-%i-%i" % (27, 46) )

    updater.add("Function updater", dummy_diagnostic)
    dc = DummyClass()
    updater.add("Method updater", dc.produce_diagnostics)
    lower = diagnostic_updater.FunctionDiagnosticTask("Lower-bound check",
        check_lower_bound)
    upper = diagnostic_updater.FunctionDiagnosticTask("Upper-bound check",
        check_upper_bound)

    bounds = diagnostic_updater.CompositeDiagnosticTask("Bound check")
    bounds.addTask(lower)
    bounds.addTask(upper)

    updater.add(bounds)

    updater.broadcast(0, "Doing important initialization stuff.")

    pub1 = rospy.Publisher("topic1", std_msgs.msg.Bool)
    pub2_temp = rospy.Publisher("topic2", std_msgs.msg.Bool)
    rospy.sleep(2) # It isn't important if it doesn't take time.
    freq_bounds = {'min':0.5, 'max':2} # If you update these values, the
    pub1_freq = diagnostic_updater.HeaderlessTopicDiagnostic("topic1", updater,
        diagnostic_updater.FrequencyStatusParam(freq_bounds, 0.1, 10))

    pub1_freq.addTask(lower) # (This wouldn't work if lower was stateful).

    updater.force_update()

    if not updater.removeByName("Bound check"):
        rospy.logerr("The Bound check task was not found when trying to remove it.")

    while not rospy.is_shutdown():
        msg = std_msgs.msg.Bool()
        rospy.sleep(0.1)

        msg.data = False
        pub1.publish(msg)
        pub1_freq.tick()

        updater.update()
