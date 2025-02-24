# Experiment models

A description of the experiment models in dx2.

## Beam

There is a hierarchy of beam classes.
- BeamBase. The base class defining beam attributes except wavelength
- MonochromaticBeam (subclass of BeamBase). Adds the wavelength field. Further subclassed by MonoXrayBeam and MonoElectronBeam, which simply set a probe name.
- PolychromaticBeam (subclass of BeamBase). Adds the wavelength range field. For support of neutron data as added to dxtbx.

## Goniometer

A single goniometer class has been defined, which can represent a multi-axis goniometer performing a 1D scan about a given rotation axis. Decomposes a multi-axis goniometer in the same way as dxtbx.

## Detector

To understand a detector, one must first understand the concept of a Panel. A Panel is an area of a detector that can be described as a single abstract unit i.e. by a single Detector matrix and detetctor parameters. This could consist of several physical detector modules (e.g. in an Eiger). A detector is then the combination of one or more Panels plus a hierarchy for the purposes of decomposition of parameters in refinement.
So far, we have only defined a Detector as simply a vector of panels without any hierarchy definition. In data processing, data is processed per panel. So the detector model needs careful thought as to how to easily allow parallelised processing but we will still need the hierarchy description for refinement of multi-panel detectors.

## Scan

A scan represents a physical measurement over a number of images. This was initially a rotation, but in dxtbx has been generalised to have a set of properties, i.e. data items that change over the 'scan' - which could be oscillation or another property. One can still initialise a scan with an image range and oscillation start & width.

Note: Thinking about this further, should we have a base scan (image range and number of images, an oscillation scan and a general scan classes?)

## Crystal

A class to represent the crystal, defined by three reciprocal space basis vectors and a space group.