import os

## \brief Monitor accessing on the dta model.
#
# This class is a meta-class that monitor data accessing for its instance
# classes.
#
# It is a monitor with a small modification.  The calling would be
# rejected when the domview_ui was locked.  It will not waiting for a
# lock.
#
# \see http://en.wikipedia.org/wiki/Monitor_%28synchronization%29
#
# All methods, of instance classes, who's name is prefixed with 'do' are
# monitored.  When a monitored method was called, monitor will try to lock
# _domview of the object.  The method is only called if successfully acquiring
# the lock, or return immediately.  The lock is released after the calling
# returned.
#
class data_monitor(type):
    def __new__(mclazz, name, bases, clazz_dict):
	debug_level = 0
	if os.environ.has_key('DATA_MONITOR_DBG'):
	    debug_level = int(os.environ['DATA_MONITOR_DBG'])
	    pass
	
	def gen_sentinel(func_name, func):
	    def sentinel(self, *args, **kws):
		if debug_level >= 1:
		    print 'calling %s.%s' % (name, func_name)
		    pass
		if debug_level >= 2:
		    print '    args: %s' % (repr(args))
		    print '    kws:  %s' % (repr(kws))
		    pass
		
		if not self._locker.lock(): # can not lock
		    if debug_level >= 1:
			print '  fault to lock'
			pass
		    return
		
		try:
		    func(self, *args, **kws)
		finally:
		    self._locker.unlock()
		    pass
		pass
	    return sentinel

        monitor_prefix = 'do'
        if clazz_dict.has_key('__data_monitor_prefix__'):
            monitor_prefix = clazz_dict['__data_monitor_prefix__']
            pass
        
	for attr_name in clazz_dict:
	    if (not attr_name.startswith(monitor_prefix)) or \
		    (not callable(clazz_dict[attr_name])):
		continue
	    clazz_dict[attr_name] = \
		gen_sentinel(attr_name, clazz_dict[attr_name])
	    pass
	
	clazz = type.__new__(mclazz, name, bases, clazz_dict)
	
	return clazz
    pass

