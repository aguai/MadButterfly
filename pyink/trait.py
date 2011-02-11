## \brief Implement descriptors for mapping attributes for traits.
#
# The instances of require map attributes of traits to corresponding
# attributes of the instance of the composition class.
#
class require(object):
    def __get__(self, instance, owner):
        if not instance:        # from a class object
            return self
        
        attrname = instance._trait_attrname_map[self]
        composite_obj = instance._trait_composite_obj
        val = getattr(composite_obj, attrname)
        return val

    def __set__(self, instance, value):
        attrname = instance._trait_attrname_map[self]
        composite_obj = instance._trait_composite_obj
        setattr(composite_obj, attrname, value)
        pass
    pass


## \brief Decorator for making a class being a trait.
#
def trait(trait_clazz):
    attrname_map = {}
    trait_clazz._trait_attrname_map = attrname_map
    
    for attr in dir(trait_clazz):
        value = getattr(trait_clazz, attr)
        if value != require:
            continue

        require_o = require()
        setattr(trait_clazz, attr, require_o)
        attrname_map[require_o] = attr
        pass

    trait_clazz._is_trait = True
    
    return trait_clazz


## \brief The function to return a proxy for a method of a trait.
#
def trait_method_proxy(trait_clazz, method):
    def trait_method_proxy_real(self, *args, **kws):
        if not hasattr(self, '_all_trait_objs'):
            # self is an instance of the class composed from traits.
            self._all_trait_objs = {}
            pass
        
        try:
            trait_obj = self._all_trait_objs[trait_clazz]
        except KeyError:
            trait_obj = trait_clazz()
            trait_obj._trait_composite_obj = self
            self._all_trait_objs[trait_clazz] = trait_obj
            pass
        
        r = method(trait_obj, *args, **kws)
        
        return r
    
    return trait_method_proxy_real


## \brief Derive and modify an existing trait.
#
def derive_trait(a_trait, composite_clazz):
    attrname_map = None
    if hasattr(composite_clazz, 'provide_traits'):
        provide_traits = composite_clazz.provide_traits
        if a_trait in provide_traits:
            provide_trait = provide_traits[a_trait]
            attrname_map = dict(a_trait._trait_attrname_map)
            attrname_map.update(provide_trait)
            pass
        pass
    
    dic = {}
    if attrname_map:
        dic['_trait_attrname_map'] = attrname_map
        pass
    
    derived = type('derived_trait', (a_trait,), dic)
    
    return derived

## \brief A decorator to make class composited from traits.
#
# The class decorated by composite must own a use_traits attribute.
#
# \verbatim
# @trait
# class trait_a(object):
#   var_a = require
#   def xxx(self): return self.var_a
#
# @trait
# class trait_b(object):
#   def ooo(self): pass
#
# @composite
# class foo(object):
#    use_traits = (trait_a, trait_b)
#
#    var_a = 'value of var_a'
#    pass
#
# obj = foo()
# \endverbatim
#
# To make a class from a set of traits.  You must decorate the class
# with the decorator 'composite'.  The class must has an attribute,
# named use_traits, to provide a list or tuple of traits.
#
# Class that defines a trait must decorated with the decorator
# 'trait'.  If the trait need to access state (varaibles) of the
# intances of composition class, it must define attributes with value
# 'require', likes what 'var_a' of trait_a does.  Then, the attributes
# would be mapped to corresponding attributes of instances of
# composition class.  For example, when you call obj.xxx(), it returns
# value of 'var_a', and attribute 'var_a' is a property that returns
# the value of 'var_a' of 'obj', an instance of class foo.
#
# By default, traits map attribute 'var_a' to 'var_a' of instances of
# composition classes.  But, you can change it by specifying the map
# in an attribute, named 'provide_traits', defined in composition
# class.  The attribute provide_traits is a dictionary mapping from
# trait class to a dictionary, named 'attrname_map' for the trait.
# The attrname_map maps require attributes of the trait to names of
# attributes of instances of the composition class.
#
def composite(clazz):
    if not hasattr(clazz, 'use_traits'):
        raise KeyError, \
            '%s has no use_trait: it must be a list of traits' % (repr(clazz))
    traits = clazz.use_traits
    
    for a_trait in traits:
        if not hasattr(a_trait, '_is_trait'):
            raise TypeError, '%s is not a trait' % (repr(a_trait))
        pass

    #
    # Check content of clazz.provide_traits
    #
    if hasattr(clazz, 'provide_traits'):
        if not isinstance(clazz.provide_traits, dict):
            raise TypeError, \
                'provide_traits of a composite must be a dictionary'
        
        provide_set = set(clazz.provide_traits.keys())
        trait_set = set(traits)
        unused_set = provide_set - trait_set
        if unused_set:
            raise ValueError, \
                'can not find %s in provide_traits' % (repr(unused_set.pop()))

        for trait, attrname_map in clazz.provide_traits.items():
            for req in attrname_map:
                if not isinstance(req, require):
                    raise TypeError, \
                        '%s is not a require: key of an ' \
                        'attribute name map must be a require' % (repr(req))
                pass
            pass
        pass
    
    #
    # Count number of appearing in all traits for every attribute name.
    #
    attrname_cnts = {}
    for a_trait in traits:
        for attr in dir(a_trait):
            if attr.startswith('_'):
                continue
            
            value = getattr(a_trait, attr)
            if value == require:
                continue
            
            attrname_cnts[attr] = attrname_cnts.setdefault(attr, 0) + 1
            pass
        pass

    if hasattr(clazz, 'method_map_traits'):
        method_map_traits = clazz.method_map_traits
    else:
        method_map_traits = {}
        pass
    
    #
    # Set a proxy for every exported methods.
    #
    derived_traits = clazz._derived_traits = {}
    for a_trait in traits:
        derived = derive_trait(a_trait, clazz)
        derived_traits[a_trait] = derived

        if a_trait in method_map_traits:
            method_map_trait = method_map_traits[a_trait]
        else:
            method_map_trait = {}
        
        for attr in dir(derived):
            if attr not in attrname_cnts: # hidden
                continue
            if attrname_cnts[attr] > 1: # conflict
                continue
            
            if hasattr(clazz, attr): # override
                continue

            value = getattr(a_trait, attr)
            if value in method_map_trait: # do it later
                continue
            
            value = getattr(derived, attr)
            
            if not callable(value):
                raise TypeError, \
                    '%s.%s is not a callable' % (repr(a_trait), attr)
            
            func = value.im_func
            proxy = trait_method_proxy(derived, func)
            setattr(clazz, attr, proxy)
            pass
        pass

    #
    # Map methods specified in method_map_traits.
    #
    for a_trait, method_map_trait in method_map_traits.items():
        if a_trait not in derived_traits:
            raise TypeError, \
                '%s is not a trait used by the composition class' % \
                (repr(a_trait))
        
        derived = derived_traits[a_trait]
        for method, attrname in method_map_trait.items():
            if not callable(method):
                raise TypeError, \
                    '%s.%s is not a callable' % (repr(a_trait), repr(method))
            func = method.im_func
            proxy = trait_method_proxy(derived, func)
            setattr(clazz, attrname, proxy)
            pass
        pass
    
    return clazz


if __name__ == '__main__':
    @trait
    class hello(object):
        msg = require
        
        def hello(self, name):
            return self.msg + ' hello ' + name
        pass

    @trait
    class bye(object):
        msg = require
        
        def bye(self, name):
            return self.msg + ' bye ' + name
        pass
    
    @composite
    class hello_bye(object):
        use_traits = (hello, bye)
        
        provide_hello = {hello.msg: 'msg1'}
        provide_traits = {hello: provide_hello}
        
        method_map_hello = {hello.hello: 'hello1'}
        method_map_traits = {hello: method_map_hello}

        msg = 'hello_bye'
        msg1 = 'hello_bye_msg1'
        pass

    o = hello_bye()
    assert o.hello1('Miky') == 'hello_bye_msg1 hello Miky'
    assert o.bye('Miky') == 'hello_bye bye Miky'
    print 'OK'
    pass
