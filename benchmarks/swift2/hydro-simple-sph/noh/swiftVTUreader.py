#TODO documentation


import os, errno
import vtuIO
import numpy as np
from typing import Union


def _clean_attribute_string(attr: str):
    """
    Clean up a string intended to be a particle attribute
    from illegal characters.

    Returns
    -------

    cleaned_attr: str
        cleaned attribute string.
    """

    cleaned_attr = attr.strip()

    # keep all illegal characters and their replacements
    # in this dict. Keys will be replaced by values.
    illegal_character_replacements = {
            "-": "_",
            " ": "_",
            }

    for key in illegal_character_replacements:
        cleaned_attr = cleaned_attr.replace(key, illegal_character_replacements[key])

    return cleaned_attr



class VTUParticleSet(object):
    """
    Container for particle data. All particle fields shall
    have the same name as specified in the .vtu files, and
    be accessible as an attribute of the instantiated object.

    The individual particle values are instances of numpy arrays.

    Methods
    -------

    show_attribute_list(self):
        prints out all available particle fields

    merge(self, other):
        merge another VTUParticleSet into this one.
    """

    def __init__(self, particle_attributes: list):
        """
        Parameters
        ----------
        particle_attributes : list of strings
            contains names of all particle fields
        """

        self._nattributes = 0
        # "translation" from particle fields in input .vtu file to
        # cleaned up attributes of this object
        self._attribute_translator = {}

        if len(particle_attributes) == 0:
            raise ValueError("Got empty particle attributes list. Can't work with that.")

        cleaned_attributes = []
        for att in particle_attributes:
            if not isinstance(att, str):
                raise ValueError("Attribute '",att,"' is not a string?")
            cleaned_attr = _clean_attribute_string(att)
            setattr(self, cleaned_attr, None)

            self._nattributes += 1
            self._attribute_translator[att] = cleaned_attr
            cleaned_attributes.append(cleaned_attr)

        self._particle_attribute_list = cleaned_attributes

        return

    def show_attribute_list(self):
        """
        Print the list of available attributes.
        """
        print("Available particle fields are:", self._particle_attribute_list)
        return

    def merge(self, other):
        """
        Merge two VTUParticleSet objects into one (into self).
        """

        if self._nattributes != other._nattributes:
            raise ValueError("Unequal number of attributes in datasets, can't merge")

        for attr in self._particle_attribute_list:
            this_arr = getattr(self, attr)
            that_arr = getattr(other, attr)
            newattr = np.concatenate((this_arr, that_arr))
            setattr(self, attr, newattr)

        return

    def __str__(self):
        return "Particle Data Container"

    def __repr__(self):
        return self.__str__()



class ParticleVTUReader (object):
    """
    Reads in particle fields from  a single .vtu file, or all .vtu
    at a given snapshot time specified in the provided .pvd file.


    Methods
    -------

    load(self):
        actually read in the data, and return the particle data as a
        VTUParticleSet object.
    """

    def __init__(self, 
                 vtufile: Union[str, None] = None,
                 snapshot_time: Union[float, None] = None,
                 pvdfile: Union[str, None] = None,
                 verbose: bool = True,
                 ):
        """
        At initialization, either the ``vtufile`` argument needs to
        be provided, or both the ``snapshot_time`` and ``pvdfile``
        arguments in order to specify what data to read.

        If ``vtufile`` is given, the ParticleVTUReader will read in a 
        single .vtu file.

        If ``snapshot_time`` and ``pvdfile`` arguments are given, the
        ParticleVTUReader will read in all .vtu files corresponding to the
        snapshot at the provided ``snapshot_time``.

        Parameters
        ----------

        vtufile: str
            path to .vtu file to be read in

        snapshot_time: float

        vtufile: str
            path to .vtu file to be read in

        verbose: bool
            how talkative the reading process should be

        """

        self._verbose = verbose
        self._mode = None
        self._vtufile = vtufile
        self._pvdfile = pvdfile
        self._snapshot_time = snapshot_time

        # First check that we have all necessary requirements
        if vtufile is None:
            if (snapshot_time is None) or (pvdfile is None):
                raise ValueError(
                        "You need to specify either the `vtufile` argument, or "+
                        "*both* the `snapshot_time` and `pvdfile` arguments so I "+
                        "know what to read."
                        )
            else:
                self._mode = "multiple"
                if self._verbose:
                    print("Setting mode to 'multiple'")
        else:
            self._mode = "single"
            if self._verbose:
                print("Setting mode to 'single'")

        return

    def load(self):
        """
        Actually read in the data.

        Returns
        -------

        particleData: VTUParticleSet
            object containing all read-in particle data as attributes.
        """

        if self._mode == "single":
            # read in the single file
            return self._read_single_vtu_file(self._vtufile)

        elif self._mode == "multiple":
            vtufiles = self._get_snapshot_vtu_files_from_pvd(self._pvdfile, self._snapshot_time)
            partdata = None
            pathprefix = os.path.dirname(self._pvdfile)
            print(pathprefix)
            for f in vtufiles:
                fullpath = os.path.join(pathprefix, f)
                new_data = self._read_single_vtu_file(fullpath)
                if partdata is None:
                    partdata = new_data
                else:
                    partdata.merge(new_data)

            return partdata

        else:
            raise ValueError("Unknown mode? How did we get here?")


    def _get_snapshot_vtu_files_from_pvd(self, pvdfilename: str, time: float):
        """
        Read in data from the .pvd file provided by pvdfilename.

        Parameters
        ----------

        pvdfilename: str
            filename to read in from

        time: float
            snapshot time to read in


        Returns
        -------

        vtufilenames: list
            list of .vtu filenames to read from
        """

        if not os.path.exists(pvdfilename):
            raise FileNotFoundError(errno.ENOENT, os.strerror(errno.ENOENT), pvdfilename)

        pvdfile = vtuIO.PVDIO(pvdfilename)
        timesteps = pvdfile.timesteps
        if self._verbose:
            print("Available snapshot time steps:", np.unique(timesteps))
        allvtufiles = pvdfile.vtufilenames

        # Find snapshots with time closest to desired output time
        timediff = np.abs(timesteps - time)
        snap_indexes = timediff == timediff.min()

        # make sure we found something
        if not snap_indexes.any():
            errmsg=f"""
            Couldn't find any snapshots? 
            Looking for snashots with time={self._snapshot_time}
            pvd file provides following times: {timesteps}
            pvd file provides following file names: {allvtufiles}
            """
            raise ValueError(errmsg)

        # check that we found the correct snapshot time
        close_enough = True
        if self._snapshot_time == 0.:
            if timediff.min() > 1.e-5:
                close_enough = False

        else:
            if np.abs(timediff.min() / self._snapshot_time) > 1.e-4:
                close_enough = False

        if self._verbose or not close_enough:
            print(f"You requested snapshot at time={self._snapshot_time}")
            print(
                    "Closest snapshot time I found is:", 
                    timesteps[snap_indexes][0], 
                    "; reading that in now"
                    )

        #  snap_indexes = np.zeros(snap_indexes.shape, dtype=bool)
        #  snap_indexes[0] = True
        vtufiles = np.array(allvtufiles)[snap_indexes]
        vtufiles = vtufiles.tolist()

        return vtufiles


    def _read_single_vtu_file(self, vtufilename: str):
        """
        Read in a single .vtu file, and return the filled out
        VTUParticleSet object containing the particle data.

        Parameters
        ----------

        vtufilename: str
            filename to read in from


        Returns
        -------

        particleData: VTUParticleSet
            object containing all read-in particle data as attributes.
        """

        if not os.path.exists(vtufilename):
            raise FileNotFoundError(errno.ENOENT, os.strerror(errno.ENOENT), vtufilename)

        vtufile = vtuIO.VTUIO(vtufilename)
        point_field_names = vtufile.get_point_field_names()

        if self._verbose:
            print("File", vtufilename, "has particle fields:", point_field_names)

        data = VTUParticleSet(point_field_names)
        
        for attr in point_field_names:
            part_fields = vtufile.get_point_field(attr)
            setattr(data, data._attribute_translator[attr], part_fields)

            if self._verbose: 
                print(
                        "File", vtufilename, "read in field", attr, 
                        "with shape", part_fields.shape
                     )

        return data


