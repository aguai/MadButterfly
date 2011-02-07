import pybExtension

class html5css3_ext(pybExtension.PYBindExtImp):
    def save(self, module, doc, filename):
        print 'save to ' + filename
        pass
    pass

extension = (html5css3_ext(),
             'net.scribboo.html5css3',
             'HTML5/CSS3 exporter',
             'output',
             {'extension': '.html',
              'mimetype': 'text/html',
              '_filetypename': 'HTML5/CSS3 (*.html)'})
