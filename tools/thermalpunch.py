
# Pouillet's law
def resistance(length, crossection, resistivity):  
    return resistivity*(length/crossection)

def power(voltage, resistance):
    return voltage**2 / resistance

# https://en.wikipedia.org/wiki/Electrical_resistivity_and_conductivity#Resistivity_and_conductivity_of_various_materials
resistivity = {
    'stainless steel': 6.90e-7,
    'carbon steel': 1.43e-7,
}

def main():
    thickness = 0.3e-3
    width = 5e-3
    area = width*thickness
    material = 'carbon steel'
    rho = resistivity[material]
    radius = 0.52
    length = 3.14*2*radius

    # thermal "die cutting" of plastics
    # using a ring constructed with a strip of plate material
    # which would be heated up by applying electricity directly
    #
    # with carbon steel needs to be thin <0.3 mm and only up to 5mm width.
    # stainless more suited, can use 0.5-0.6 mm and 10 mm width.
    #
    # should first test applying puncher cold - right after thermoforming, while material is still hot
    R = resistance(length, area, rho)
    print '%s w=%.2f mm, thickness=%.2f mm' % (material, width*1000, thickness*1000)
    print 'length=%.2f meters' % (length,)
    print 'R=%.3f ohm' % (R,)

    voltage = 12
    print 'U=%.2f V' % (voltage,)
    print 'P=%.2f watts, I=%.2f ampere' % (power(voltage, R), voltage/R)

if __name__ == '__main__':
    main()
