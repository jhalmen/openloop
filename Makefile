##
## This file is part of the openloop hardware looper project.
##
## Copyright (C) 2018  Jonathan Halmen <jonathan@halmen.org>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program. If not, see <http://www.gnu.org/licenses/>.
##
BINARY = miniblink

OPENCM3_DIR = libopencm3
DEVICE = stm32f401rct6
# include ./libopencm3/ld/Makefile.linker

include libopencm3.target.mk
