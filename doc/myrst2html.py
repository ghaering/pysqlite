#!/usr/bin/env python

import SilverCity
import docutils.parsers.rst
import StringIO

def code_block( name, arguments, options, content, lineno,
             content_offset, block_text, state, state_machine ):
  """
  The code-block directive provides syntax highlighting for blocks
  of code.  It is used with the the following syntax::
  
  .. code-block:: CPP
     
    #include <iostream>
    
    int main( int argc, char* argv[] )
    {
      std::cout << "Hello world" << std::endl;
    }
    
  The directive requires the name of a language supported by SilverCity
  as its only argument.  All code in the indented block following
  the directive will be colourized.  Note that this directive is only
  supported for HTML writers.

  The directive can also be told to include a source file directly::

  .. code-block::
     :language: Python
     :source-file: ../myfile.py

  You cannot both specify a source-file and include code directly.
  """

  try:
    language = arguments[0]
  except IndexError:
    language = options['language']

  if content and 'source-file' in options:
    error = state_machine.reporter.error( "You cannot both specify a source-file and include code directly.",
                                          docutils.nodes.literal_block(block_text,block_text), line=lineno)
    return [error]

  if not content:
    try:
      content = [line.rstrip() for line in file(options['source-file'])]
    except KeyError:
      # source-file was not specified
      pass
    except IOError:
      error = state_machine.reporter.error( "Could not read file %s."%options['source-file'],
                                            docutils.nodes.literal_block(block_text,block_text), line=lineno)
      return [error]
  try:
    module = getattr(SilverCity, language)
    generator = getattr(module, language+"HTMLGenerator")
  except AttributeError:
    error = state_machine.reporter.error( "No SilverCity lexer found "
      "for language '%s'." % language, 
      docutils.nodes.literal_block(block_text, block_text), line=lineno )
    return [error]
  io = StringIO.StringIO()
  generator().generate_html( io, '\n'.join(content) )
  html = '<div class="code-block">\n%s\n</div>\n' % io.getvalue()
  raw = docutils.nodes.raw('',html, format = 'html')
  return [raw]

#code_block.arguments = (1,0,0)
code_block.arguments = (0,2,1)
code_block.options = {'language' : docutils.parsers.rst.directives.unchanged,
                      'source-file' : docutils.parsers.rst.directives.path,}
code_block.content = 1
  
# Simply importing this module will make the directive available.
docutils.parsers.rst.directives.register_directive( 'code-block', code_block )

if __name__ == "__main__":
  import docutils.core
  docutils.core.publish_cmdline(writer_name='html')

