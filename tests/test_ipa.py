import unittest
import ipaddress
from patricia26 import Patricia26Int  # Adjust import based on your build structure

class TestPatricia26IpaExtensions(unittest.TestCase):

    def setUp(self):
        """Instantiate a tree and pre-load mock data structures."""
        self.tree_v4 = Patricia26Int()
        self.tree_v6 = Patricia26Int()
        
        # Core testing networks
        self.v4_net = ipaddress.ip_network("192.168.1.0/24")
        self.v4_host = ipaddress.ip_address("192.168.1.50")
        
        self.v6_net = ipaddress.ip_network("2001:db8::/32")
        self.v6_host = ipaddress.ip_address("2001:db8::100")

        # Map payloads to our native ipaddress keys
        self.tree_v4[self.v4_net] = "v4_subnet_data"
        self.tree_v6[self.v6_net] = "v6_subnet_data"

    def test_lookup_ipa_exact_and_lpm(self):
        """Verify single LPM tracking matches network layouts directly."""
        # 1. Exact match on network objects
        self.assertEqual(self.tree_v4.lookup(self.v4_net), "v4_subnet_data")
        self.assertEqual(self.tree_v6.lookup(self.v6_net), "v6_subnet_data")
        
        # 2. LPM evaluation for individual host addresses within blocks
        self.assertEqual(self.tree_v4.lookup(self.v4_host), "v4_subnet_data")
        self.assertEqual(self.tree_v6.lookup(self.v6_host), "v6_subnet_data")
        
        # 3. Miss condition
        miss_host = ipaddress.ip_address("10.0.0.1")
        self.assertIsNone(self.tree_v4.lookup(miss_host))

    def test_bulk_lookup_ipa(self):
        """Ensure batch queries resolve instantly without list allocation errors."""
        query_list_v4 = [
            self.v4_host,
            ipaddress.ip_address("10.10.10.10"),  # Miss
            self.v4_net
        ]

        query_list_v6 = [
            ipaddress.ip_address("001:f00d::100"),  # Miss
            self.v6_host,
        ]
        
        expected_results_v4 = [
            "v4_subnet_data",
            None,
            "v4_subnet_data"
        ]

        expected_results_v6 = [
            None,
            "v6_subnet_data"
        ]
        
        results_v4 = self.tree_v4.bulk_lookup(query_list_v4)
        self.assertEqual(results_v4, expected_results_v4)

        results_v6 = self.tree_v6.bulk_lookup(query_list_v6)
        self.assertEqual(results_v6, expected_results_v6)

    def test_lookup_lpm_ipa_tuple_unpacking(self):
        """Verify tuple returns provide (ipaddress_network, data)."""
        # Test accurate network structure unwrapping
        matched_net, data = self.tree_v4.lookup_lpm(self.v4_host)
        self.assertEqual(matched_net, self.v4_net)
        self.assertEqual(data, "v4_subnet_data")
        
        # Test miss structure mapping
        miss_net, miss_data = self.tree_v4.lookup_lpm(ipaddress.ip_address("10.0.0.1"))
        self.assertIsNone(miss_net)
        self.assertIsNone(miss_data)

    def test_get_prefix_ipa(self):
        """Verify extraction of the tracking network key object."""
        net_key = self.tree_v4.get_prefix(self.v4_host)
        self.assertEqual(net_key, self.v4_net)
        self.assertIsInstance(net_key, ipaddress.IPv4Network)

    def test_parent_and_children_ipa(self):
        """Test hierarchical traversal using native objects."""
        # Setup parent/child relationships
        parent_net = ipaddress.ip_network("10.0.0.0/8")
        child_net = ipaddress.ip_network("10.1.0.0/16")
        
        self.tree_v4[parent_net] = "parent"
        self.tree_v4[child_net] = "child"
        
        # Evaluate parent mapping
        self.assertEqual(self.tree_v4.parent(child_net), parent_net)
        
        # Evaluate child array mapping
        children = self.tree_v4.children(parent_net)
        self.assertIn(child_net, children)

    def test_prefixes_ipa_flat_generation(self):
        """Confirm full tree extraction outputs valid ipaddress objects."""
        all_keys_v4 = self.tree_v4.prefixes()
        all_keys_v6 = self.tree_v6.prefixes()
        
        self.assertIn(self.v4_net, all_keys_v4)
        self.assertIn(self.v6_net, all_keys_v6)
        
        for k in all_keys_v4:
            self.assertTrue(isinstance(k, ipaddress.IPv4Network))

        for k in all_keys_v6:
            self.assertTrue(isinstance(k, ipaddress.IPv6Network))

if __name__ == "__main__":
    unittest.main()
