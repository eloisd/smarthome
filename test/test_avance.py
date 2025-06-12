import smbus
import time

def test_capteur_simple():
    bus = smbus.SMBus(1)
    address = 0x74
    
    print("=== Test simple SEN0307 ===")
    
    # Test de présence
    try:
        bus.read_byte(address)
        print("✓ Capteur détecté à l'adresse 0x74")
    except Exception as e:
        print(f"✗ Erreur détection: {e}")
        return
    
    # Test différentes méthodes de lecture
    methods = [
        ("Méthode 1: write_byte(0x01) puis read_i2c_block", test_method1),
        ("Méthode 2: read_i2c_block direct", test_method2),
        ("Méthode 3: write_byte_data puis read_byte_data", test_method3),
        ("Méthode 4: read_word_data", test_method4),
    ]
    
    for name, method in methods:
        print(f"\n--- {name} ---")
        for i in range(3):
            try:
                distance = method(bus, address)
                print(f"Tentative {i+1}: {distance} mm")
                if distance and 20 <= distance <= 4500:
                    print(f"✓ Méthode valide! Distance: {distance/10:.1f} cm")
                    return method
            except Exception as e:
                print(f"Tentative {i+1}: Erreur - {e}")
            time.sleep(0.2)
    
    print("\n❌ Aucune méthode ne fonctionne")
    bus.close()

def test_method1(bus, address):
    """Méthode originale"""
    bus.write_byte(address, 0x01)
    time.sleep(0.12)
    data = bus.read_i2c_block_data(address, 0x00, 2)
    return (data[0] << 8) | data[1]

def test_method2(bus, address):
    """Lecture directe"""
    data = bus.read_i2c_block_data(address, 0x00, 2)
    return (data[0] << 8) | data[1]

def test_method3(bus, address):
    """Avec registres séparés"""
    bus.write_byte_data(address, 0x00, 0x01)
    time.sleep(0.1)
    high = bus.read_byte_data(address, 0x00)
    low = bus.read_byte_data(address, 0x01)
    return (high << 8) | low

def test_method4(bus, address):
    """Lecture word directe"""
    data = bus.read_word_data(address, 0x00)
    # Inverser les bytes si nécessaire
    return ((data & 0xFF) << 8) | ((data >> 8) & 0xFF)

if __name__ == "__main__":
    test_capteur_simple()
EOF
