import _smart

class Smart(object):
    def __init__(self, dev_path):
        self.__dev_path = dev_path
        self.__smart = None
        self.__opened = False

    def __enter__(self):
        self.open()

        return self
    
    def open(self):
        self.__smart = _smart.Smart(self.__dev_path)
        self.__opened = True

    def __exit__(self, type, value, tb):
        self.__smart.close()
        return False

    @property
    def opened(self):
        return self.__opened

    @property
    def size(self):
        if not self.opened:
            self.open()
        return self.__smart.get_size(False)

    @property
    def size_text(self):
        if not self.opened:
            self.open()
        return self.__smart.get_size(human_readable = True)

    @property
    def status(self):
        if not self.opened:
            self.open()
        return self.__smart.smart_status()

    @property
    def identify_data(self):
        if not self.opened:
            self.open()
        return self.__smart.get_identify()

    @property
    def sleep_mode(self):
        if not self.opened:
            self.open()
        return self.__smart.check_sleep_mode()

    @property
    def available(self):
        if not self.opened:
            self.open()
        return self.__smart.smart_is_available()

    @property
    def bad_sectors(self):
        if not self.opened:
            self.open()
        return self.__smart.get_bad_sectors()

    @property
    def power_cycle(self):
        if not self.opened:
            self.open()
        return self.__smart.get_power_cycle()

    @property
    def power_on(self):
        if not self.opened:
            self.open()
        return self.__smart.get_power_on()

    @property
    def temperature(self):
        if not self.opened:
            self.open()
        return self.__smart.get_temperature()

    @property
    def overall_health(self):
        if not self.opened:
            self.open()
        return self.__smart.get_overall()

    @property
    def overall_health_text(self):
        if not self.opened:
            self.open()
        return self.__smart.get_overall(human_readable = True)

    def read_data(self):
        if not self.opened:
            self.open()
        self.__smart.read_data()
    
    @property
    def info(self):
        if not self.opened:
            self.open()
        return self.__smart.get_info()

    @property
    def info_text(self):
        if not self.opened:
            self.open()
        return self.__smart.get_info(human_readable = True)

    def get_attributes(self):
        if not self.opened:
            self.open()
        return self.__smart.get_attributes()

    def close(self):
        if self.opened:
            self.__smart.close()

    def is_value_reached(self, id, value):
        if not self.opened:
            self.open()
        attr_dict = self.get_attributes()
        try:
            attr = attr_dict[id]
            if attr['value'] >= value:
                return True
            else:
                return False
        except KeyError:
            raise AttributeError('Invalid S.M.A.R.T ID: {id}. May be unsupported?'.format(id = id))