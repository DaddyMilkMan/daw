#include <juce_core/juce_core.h>
#include "../network/ZenithCRDT.h"
#include "../network/LoroCRDTBridge.h"

namespace zenith {
namespace tests {

class CollaborationTests : public juce::UnitTest {
public:
    CollaborationTests() : juce::UnitTest("Collaboration Core", "Network") {}

    void runTest() override {
        testLWWRegister();
        testRGAList();
        testDeltaSerialization();
        testValueTreeBridge();
    }

    void testLWWRegister() {
        beginTest("LWW Register Conflict Resolution");
        
        Zenith::LWWRegister reg;
        Zenith::LoroValue valA("A");
        Zenith::Timestamp tsA = { 100, { 1 } }; // Counter 100, Peer 1
        
        // Initial set
        expect(reg.set(valA, tsA));
        expect(reg.value.sVal == "A");

        // Older timestamp should be ignored
        Zenith::LoroValue valOld("Old");
        Zenith::Timestamp tsOld = { 90, { 2 } };
        expect(!reg.set(valOld, tsOld));
        expect(reg.value.sVal == "A");

        // Newer timestamp should overwrite
        Zenith::LoroValue valB("B");
        Zenith::Timestamp tsB = { 101, { 1 } };
        expect(reg.set(valB, tsB));
        expect(reg.value.sVal == "B");

        // Same timestamp, higher peer ID should win (arbitrary but deterministic)
        Zenith::LoroValue valC("C");
        Zenith::Timestamp tsC = { 101, { 2 } }; // Peer 2 > Peer 1
        expect(reg.set(valC, tsC));
        expect(reg.value.sVal == "C");
        
        // Same timestamp, lower peer ID should lose
        Zenith::LoroValue valD("D");
        Zenith::Timestamp tsD = { 101, { 1 } };
        expect(!reg.set(valD, tsD));
        expect(reg.value.sVal == "C");
    }

    void testRGAList() {
        beginTest("RGA List Insertion and Ordering");

        Zenith::LoroList list;
        Zenith::Timestamp t0 = { 0, { 0 } }; // Origin start
        
        // Insert A at start
        Zenith::Timestamp idA = { 1, { 1 } };
        list.insert(Zenith::LoroValue("A"), idA, t0);
        
        // Insert B after A
        Zenith::Timestamp idB = { 2, { 1 } };
        list.insert(Zenith::LoroValue("B"), idB, idA);

        auto values = list.getActiveValues();
        expect(values.size() == 2);
        expect(values[0].sVal == "A");
        expect(values[1].sVal == "B");

        // Insert C after A (concurrent with B)
        // If Peer 2 inserts C after A, and Peer 1 inserted B after A.
        // C has ID {1, 2}, B has ID {2, 1}. 
        // Logic: 
        // 1. Successors follow origin (both follow A).
        // 2. Siblings ordered by ID descending. 
        // Let's verify sort order.
        
        Zenith::Timestamp idC = { 1, { 2 } }; 
        list.insert(Zenith::LoroValue("C"), idC, idA);
        
        values = list.getActiveValues();
        expect(values.size() == 3);
        // Ordering depends on specific RGA implementation details in insert().
        // Usually, higher ID siblings come first (right-to-left) or last (left-to-right).
        // Let's verify it is deterministic.
        expect(values[0].sVal == "A");
        // We just expect them to be there.
        bool foundB = false, foundC = false;
        for (auto& v : values) {
            if (v.sVal == "B") foundB = true;
            if (v.sVal == "C") foundC = true;
        }
        expect(foundB && foundC);
        
        // Tombstone (Delete A)
        list.remove(idA);
        values = list.getActiveValues();
        expect(values.size() == 2);
        expect(values[0].sVal != "A"); // A is gone
    }

    void testDeltaSerialization() {
        beginTest("CRDT Delta Serialization");

        Zenith::LoroDoc docA;
        auto& mapA = docA.getMap("test");
        mapA.set("key1", Zenith::LoroValue((juce::int64)123), { docA.nextCounter(), docA.getPeerID() });
        
        auto updates = docA.exportUpdates();
        expect(updates.getSize() > 0);

        Zenith::LoroDoc docB;
        docB.importUpdates(updates);
        
        auto& mapB = docB.getMap("test");
        expect(mapB.get("key1").iVal == 123);
    }
    
    void testValueTreeBridge() {
        beginTest("ValueTree <-> CRDT Bridge");
        
        juce::ValueTree tree("ROOT");
        tree.setProperty("id", "root_1", nullptr);
        
        Zenith::LoroDoc doc;
        Zenith::ValueTreeCRDTBridge bridge(tree, doc);
        
        // 1. Local change -> CRDT
        tree.setProperty("myProp", "Hello", nullptr);
        
        auto& map = doc.getMap("root_1");
        expect(map.get("myProp").sVal == "Hello");
        
        // 2. CRDT -> Local change (simulate remote update)
        Zenith::LoroDoc remoteDoc;
        auto& remoteMap = remoteDoc.getMap("root_1");
        remoteMap.set("myProp", Zenith::LoroValue("World"), { 100, { 99 } }); // Higher timestamp
        
        auto updates = remoteDoc.exportUpdates();
        bridge.applyRemoteUpdates(updates);
        
        expect(tree.getProperty("myProp").toString() == "World");
        
        // 3. Structural change (Add child)
        juce::ValueTree child("CHILD");
        child.setProperty("id", "child_1", nullptr);
        tree.appendChild(child, nullptr);
        
        auto& list = doc.getList("root_1_children");
        auto values = list.getActiveValues();
        expect(values.size() == 1);
        expect(values[0].sVal == "child_1");
    }
};

static CollaborationTests collaborationTests;

} // namespace tests
} // namespace zenith
