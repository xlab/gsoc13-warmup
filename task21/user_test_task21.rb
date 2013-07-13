#!/usr/bin/env ruby

# Task 2.1
# Simple tests
# by Maxim Kouprianov

DEVICE = '/dev/poums0'

if not File.exist? DEVICE
  puts "Device #{DEVICE} does not exist"
 # exit
end

# truncate
File.open(DEVICE, File::TRUNC)

# test append & write
File.open(DEVICE, File::WRONLY | File::APPEND) do |f|
  20.times do
    f.write('*')
  end
end

# test reading
File.open(DEVICE, File::RDONLY) do |f|
  puts f.read()
end

# test seek
File.open(DEVICE, File::WRONLY) do |f|
  f.seek(7, IO::SEEK_SET)
  f.write(' Xlab ')
end

# dd bytecounting
`dd if=#{DEVICE} of=copy.txt`
if File.exist?('copy.txt')
  puts File.read('copy.txt')
  File.delete('copy.txt')
else
  puts 'dd test gone wrong'
end
