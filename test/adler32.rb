#!/usr/bin/env ruby

require 'zlib'

$algo = File.basename($0, '.rb').to_sym

def checksum io, bufsiz: 1 << 16
  r = nil
  while (chunk = io.read bufsiz)
    r = Zlib.send $algo, chunk, r
  end
  r.to_s 16
end

def Warning.warn w; $stderr.print $algo.to_s + ': '; super w; end

if ARGV.size != 0
  exit_code = 0
  ARGV.each do |file|
    if File.directory? file
      warn "#{file}: is a directory"
      exit_code = 1
      next
    end
    begin
      io = open file, 'rb'
    rescue
      warn "#{file}: #{$!}"
      exit_code = 1
      next
    end

    puts '%s  %s' % [checksum(io), File.basename(file)]
    io.close
  end

  exit exit_code
else

  puts '%s  %s' % [checksum($stdin), '-']
end
