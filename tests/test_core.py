import pytest
from patricia26 import Patricia26

def test_basic_assignment_and_lookup():
    """Verifies standard dictionary lookup style operations."""
    pyt = Patricia26()
    pyt["192.168.1.0/24"] = "Local Lan"
    pyt['fe80:beef::/64'] = 'abc'
    assert pyt["192.168.1.0/24"] == "Local Lan"
    assert pyt["192.168.1.50"] == "Local Lan"
    assert pyt['fe80:beef::/64'] == "abc"

def test_freeze_and_thaw():
    """Verifies that structural locking functions properly."""
    pyt = Patricia26()
    pyt["192.168.1.0/24"] = "Allowed"
    
    pyt.freeze()
    with pytest.raises(RuntimeError, match="Cannot modify a frozen Patricia26 tree"):
        pyt["10.0.0.0/8"] = "Blocked"
        
    pyt.thaw()
    pyt["10.0.0.0/8"] = "Thawed Assignment"
    assert pyt["10.0.0.0/8"] == "Thawed Assignment"

def test_save_loop():
    """Verifies tree pointer loop generator output matches inputs exactly."""
    pyt = Patricia26()
    inserted_cidrs = {"10.0.0.0/24", "192.168.1.0/24"}
    
    for cidr in inserted_cidrs:
        pyt[cidr] = True
        
    extracted_keys = set(pyt)
    assert extracted_keys == inserted_cidrs

def test_save_insert_loop():
    """Verifies tree pointer loop generator output matches inputs exactly."""
    pyt = Patricia26()
    inserted_cidrs = {"10.0.0.0/24", "192.168.1.0/24"}
    
    for cidr in inserted_cidrs:
        pyt.insert(cidr, True)
        
    extracted_keys = set(pyt)
    assert extracted_keys == inserted_cidrs

def test_lookup_loop():
    """Verifies lookup data matches inputs exactly."""
    pyt = Patricia26()
    inserted_cidrs = {"10.0.0.0/24", "192.168.1.0/24"}
    
    for cidr in inserted_cidrs:
        pyt[cidr] = cidr

    extracted: list[str] = []
    for cidr in inserted_cidrs:
        extracted.append(pyt.lookup(cidr))
        #extracted.append(pyt[cidr])
        
    extracted_keys = set(extracted)
    assert extracted_keys == inserted_cidrs

def test_lookup_dict_loop():
    """Verifies lookup data matches inputs exactly."""
    pyt = Patricia26()
    inserted_cidrs = {"10.0.0.0/24", "192.168.1.0/24"}
    
    for cidr in inserted_cidrs:
        pyt[cidr] = cidr

    extracted: list[str] = []
    for cidr in inserted_cidrs:
        extracted.append(pyt[cidr])
        #extracted.append(pyt[cidr])
        
    extracted_keys = set(extracted)
    assert extracted_keys == inserted_cidrs

def test_bulk_lookup():
    """Verifies lookup data matches inputs exactly."""
    pyt = Patricia26()
    inserted_cidrs = {"10.0.0.0/24", "192.168.1.0/24"}
    
    for cidr in inserted_cidrs:
        pyt[cidr] = cidr

    extracted: list[str] = []
    extracted = pyt.bulk_lookup(list(inserted_cidrs))
        
    extracted_keys = set(extracted)
    assert extracted_keys == inserted_cidrs

def test_has_prefix():
    """Verifes that has_key correctly matches"""
    pyt = Patricia26()
    pfx = "192.168.0.0/16"
    val = "LAN"
    pyt[pfx] = val

    should_match = pyt.has_prefix(pfx)
    should_not_match = pyt.has_prefix("192.168.1.1")
    assert should_match and not should_not_match

def test_contains():
    """Verifies that the "in: operator works correctly"""
    pyt = Patricia26()
    pfx = "192.168.0.0/16"
    val = "LAN"
    pyt[pfx] = val

    should_match = pfx in pyt
    should_not_match = "10.0.1.1" in pyt
    assert should_match 
    assert not should_not_match

