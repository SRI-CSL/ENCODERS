/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

#include <libcpphaggle/Platform.h>
#include "SendPriorityManager.h"
#include "XMLMetadata.h"

class SendPriorityElement;
class PartialOrderCombinerOrdered;
class PartialOrderFIFO;
class PartialOrderPriorityAttribute;
class TopologicalSorter;

/**
 *  Construct a new node in the Hesse Graph, given a list of equivalent
 *  data objects as send priority elements.
 */
PartialOrderElement::PartialOrderElement(List<SendPriorityElement *> *objs /**< [in] The set of equivalent send priority elements to be wrapped. **/) :
    parentCount(0)
{
    if (!objs) {
        is_inf = true;
        return;
    }
    is_inf = false;

    for (List<SendPriorityElement *>::iterator it = objs->begin(); it != objs->end(); it++) {
        equivObjs.push_back(*it);
    }
}

/**
 * Construct a new node in the Hesse Graph, given an individual send priorty
 * element.
 */
PartialOrderElement::PartialOrderElement(SendPriorityElement *se /**< [in] An individual send priorty element to be wrapped. **/) :
    parentCount(0) {

    if (!se) {
        is_inf = true;
        return;
    }
    is_inf = false;

    equivObjs.push_back(se);
}

/**
 * Get the list of send priority elements whose data objects are equivalent.
 */
void
PartialOrderElement::getEquivObjects(List<SendPriorityElement *> *o_equivObjects /**< [out] The list to fill with equivalent send priority elements. */) {
    if (!o_equivObjects) {
        HAGGLE_ERR("NULL data objects\n");
        return;
    }
    for (List<SendPriorityElement *>::iterator it = equivObjs.begin(); it != equivObjs.end(); it++) {
        o_equivObjects->push_back(*it);
    }
}

/**
 * Add a child partial order element--the child must be immediately
 * succeeding the current element.
 */
void
PartialOrderElement::addChild(PartialOrderElement *childToAdd /**< [out] The child to add to the children list. */) {
    if (!childToAdd) {
        HAGGLE_ERR("Child missing.\n");
        return;
    }
    bool in_list = false;
    for (List<PartialOrderElement *>::iterator it = children.begin(); it != children.end(); it++) {
        if (childToAdd == (*it)) {
            in_list = true;
            break;
        }
    }
    if (!in_list) {
        children.push_back(childToAdd);
        childToAdd->incrementParentCount();
    }
}

/**
 * Add a list of children.
 */
void
PartialOrderElement::addChildren(List<PartialOrderElement *> *childrenToAdd /**< [in] Add a list of immediate children to this element. */) {
    if (!childrenToAdd) {
        HAGGLE_ERR("NULL children to add\n");
        return;
    }
    for (List<PartialOrderElement *>::iterator it = childrenToAdd->begin(); it != childrenToAdd->end(); it++) {
        addChild(*it);
    }
}

/**
 * Add an equivalent send priority element.
 */
void
PartialOrderElement::addEquivalent(SendPriorityElement *se /**< [in] An individual equivalent send priority element to be added. */) {
    if (!se) {
        HAGGLE_ERR("NULL data object to add.\n");
        return;
    }
    bool in_list = false;
    for (List<SendPriorityElement *>::iterator it = equivObjs.begin(); it != equivObjs.end(); it++) {
        if ((*it) == se) {
            in_list = true;
            break;
        }
    }
    if (!in_list) {
        equivObjs.push_back(se);
    }
}

/**
 * Add a list of equivalent send priorty elements.
 */
void
PartialOrderElement::addEquivalents(List<SendPriorityElement *> *seles /**< [in] Add a list of equivalent send priority elements. */) {
    if (!seles) {
        HAGGLE_ERR("No equivalent send elements to add\n");
        return;
    }
    for (List<SendPriorityElement *>::iterator it = seles->begin(); it != seles->end(); it++) {
        addEquivalent(*it);
    }
}

/**
 * Get the number of equivalent data objects.
 */
int
PartialOrderElement::getNumEquivalent() {
    return equivObjs.size();
}

/**
 * Remove a send priorty element from the equivalent list and return it.
 * @return The popped equivalent send priority element.
 */
SendPriorityElement *
PartialOrderElement::popEquivalent() {
    List<SendPriorityElement *>::iterator it = equivObjs.begin();
    if (it == equivObjs.end()) {
        return NULL;
    }
    SendPriorityElement *se = (*it);
    if (!se) {
        HAGGLE_ERR("NULL send element\n");
    }
    equivObjs.erase(it);
    return se;
}

/**
 * Return a send priorty element from the equivalent list.
 * @return The peeked equivalent send priority element.
 */
SendPriorityElement *
PartialOrderElement::peekEquivalent() {
    List<SendPriorityElement *>::iterator it = equivObjs.begin();
    if (it == equivObjs.end()) {
        return NULL;
    }
    SendPriorityElement *se = (*it);
    if (!se) {
        HAGGLE_ERR("NULL send element\n");
    }
    return se;
}

/**
 * Get the list of immediate children.
 */
void
PartialOrderElement::getChildren(List<PartialOrderElement *> *o_children /**< [out] The list to populate with immediate children of the current element. */) {
    if (!o_children) {
        HAGGLE_ERR("NULL data objects\n");
        return;
    }
    for (List<PartialOrderElement *>::iterator it = children.begin(); it != children.end(); it++) {
        o_children->push_back(*it);
    }
}

/**
 * The list of child partial order elements to remove.
 */
void
PartialOrderElement::removeChild(PartialOrderElement *childToRemove /**< [in] The children to remove. */) {
    if (!childToRemove) {
        HAGGLE_ERR("Missing child to remove.\n");
        return;
    }
    for (List<PartialOrderElement *>::iterator it = children.begin(); it != children.end(); it++) {
        if ((*it) == childToRemove) {
            children.erase(it);
            childToRemove->decrementParentCount();
            break;
        }
    }
}

/**
 * Remove a child from the children list and return it.
 */
PartialOrderElement *
PartialOrderElement::popChild() {
    List<PartialOrderElement *>::iterator it = children.begin();
    if (it == children.end()) {
        return NULL;
    }
    children.erase(it);
    return (*it);
}

/**
 * Remove children that are in a list of elements from the children list.
 */
void
PartialOrderElement::removeChildren(List<PartialOrderElement *> *childrenToRemove) {
    if (!childrenToRemove) {
        HAGGLE_ERR("No children to remove\n");
        return;
    }
    for (List<PartialOrderElement *>::iterator it = childrenToRemove->begin(); it != childrenToRemove->end(); it++) {
        removeChild(*it);
    }
}

/**
 * Get the number of children.
 */
int
PartialOrderElement::getNumChildren() {
    return children.size();
}

/**
 * Clone the current element and all of its children.
 */
PartialOrderElement *
PartialOrderElement::makeCopyWithoutParent() {
    PartialOrderElement *copy = new PartialOrderElement(&equivObjs);
    for (List<PartialOrderElement *>::iterator it = children.begin(); it != children.end(); it++) {
        PartialOrderElement *childCopy = (*it)->makeCopyWithoutParent();
        copy->addChild(childCopy);
    }
    return copy;
}

/**
 * Increment the parent count, used when this element is added as a child
 * for another parent.
 */
void
PartialOrderElement::incrementParentCount() {
    parentCount++;
}

/**
 * Decrement the parent count, used when this element is removed as a child
 * from another parent.
 */
void
PartialOrderElement::decrementParentCount() {
    parentCount--;
}

/**
 * Get the parent count.
 * @return The number of parents this element has.
 */
int
PartialOrderElement::getParentCount() {
    return parentCount;
}

/**
 * Check with the element is an infinity element.
 * @return True if it is an infinity element, False othewise.
 */
bool
PartialOrderElement::isInf() {
    return is_inf;
}


/**
 * State for sending a data object to a destination using a protocol.
 */
class SendPriorityElement
{
public:
    DataObjectRef dObj; /**< The data object to be sent. */
    NodeRef node; /**< The receiving node. */
    Timeval createTime;
    EventType sendCallback;

    /**
     * Construct a new send priority element given a data object, rx node,
     *  peer interface, and protocol.
     */
    SendPriorityElement(
            DataObjectRef _dObj, /**< The data object to be sent. */
            NodeRef _node /**< The node receiving the data object. */,
            EventType _sendCallback = 0) :
        dObj(_dObj),
        node(_node),
        createTime(Timeval::now()),
        sendCallback(_sendCallback) {};

    /**
     * Deconstruct the send priority element.
     */
    ~SendPriorityElement()
    {
        //Timeval now = Timeval::now();
        //HAGGLE_DBG("Element age: %d\n", (int)(now-createTime).getTimeAsMilliSeconds());
    }

    /**
     * Get the data object to be sent.
     * @return The data object to be sent.
     */

    DataObjectRef getDataObject() { return dObj; }
    /**
     * Get the node that the data object is sent to.
     * @return The node that the data object is sent to.
     */

    NodeRef getNode() { return node; }

    /**
     * Get the create time of the element.
     * @return The time at which the element was created.
     */
    Timeval getCreateTime() {
        return createTime;
    }

    EventType getSendCallback() {
        return sendCallback;
    }
};

PartialOrder::PartialOrder() {};
PartialOrder::~PartialOrder() {}

int
PartialOrder::compare(PartialOrderElement *a, PartialOrderElement *b)
{
    return PartialOrder::NC;
}

bool
PartialOrder::isInf(PartialOrderElement *a)
{
    if (!a) {
        HAGGLE_ERR("Missing element.\n");
        return false;
    }
    return a->isInf();
}

/**
 * A total order based on creation time of the Partial Order Element.
 * This class provides a standard FIFO queue, and is mainly used
 * for backwards compatibility in compisition with another partial order.
 */
class PartialOrderFIFO : public PartialOrder
{
public:
    /**
     * Compare two elements by queue insertion time.
     * @return LT if a insert date is < b insert date, GT if a insert date > b insert date, EQ if a insert date = b insert date, NC if error.
     */
    int compare(PartialOrderElement *a /**< [in] The first element to the binary relation. */, PartialOrderElement *b /** [in] The second element to the binary relation. */) {
         if (isInf(a)) {
            return PartialOrder::GT;
        }
        if (isInf(b)) {
            return PartialOrder::LT;
        }
        if (!a || !b) {
            HAGGLE_ERR("Data object missing.\n");
            return PartialOrder::NC;
        }
        SendPriorityElement *a_se = a->peekEquivalent();
        if (!a_se) {
            HAGGLE_ERR("Could not get send priority element a.\n");
            return PartialOrder::NC;
        }
        DataObjectRef a_do = a_se->getDataObject();
        if (!a_do) {
            HAGGLE_ERR("Could not get data object a.\n");
            return PartialOrder::NC;
        }
        SendPriorityElement *b_se = b->peekEquivalent();
        if (!b_se) {
            HAGGLE_ERR("Could not get send priority element b.\n");
            return PartialOrder::NC;
        }
        DataObjectRef b_do = b_se->getDataObject();
        if (!b_do) {
            HAGGLE_ERR("Could not get data object a.\n");
            return PartialOrder::NC;
        }

        if (a_se->getCreateTime() < b_se->getCreateTime()) {
            return PartialOrder::GT;
        }
        if (a_se->getCreateTime() == b_se->getCreateTime()) {
            return PartialOrder::EQ;
        }
        return PartialOrder::LT;
    }

    static PartialOrder *create(Metadata *m /**< [in] Metadata containing configuration setttings for the partial order. */) {
        HAGGLE_DBG("Loaded Partial Order FIFO.\n");
        return new PartialOrderFIFO();
    }
};

/**
 * A partial order that combines two partial orders, by specifiying
 * their order.
 */
class PartialOrderCombinerOrdered : public PartialOrder
{
private:
    PartialOrder *high_po;
    PartialOrder *low_po;
public:
    /**
     * Instantiate a new partial order combiner ordered.
     */

    PartialOrderCombinerOrdered(
        PartialOrder *_high_po,  /**< [in] High priority partial order. */
        PartialOrder *_low_po /**< [in] Low priority partial order. */) : high_po(_high_po), low_po(_low_po) {};

    /**
     * Free a new partial order combiner ordered.
     */
    ~PartialOrderCombinerOrdered() {
        if (high_po) {
            delete high_po;
        }
        if (low_po) {
            delete low_po;
        }
    };

    /**
     * Compare two elements by queue insertion time.
     * @return LT if a insert date is < b insert date, GT if a insert date > b insert date, EQ if a insert date = b insert date, NC if error.
     */
    int compare(PartialOrderElement *a /**< [in] The first element to the binary relation. */, PartialOrderElement *b /** [in] The second element to the binary relation. */) {
        if (!a || !b) {
            HAGGLE_ERR("Data object missing.\n");
            return PartialOrder::NC;
        }
        if (isInf(a)) {
            return PartialOrder::GT;
        }
        if (isInf(b)) {
            return PartialOrder::LT;
        }
        int high_cmp = high_po->compare(a, b);
        int low_cmp = low_po->compare(a, b);
        if (high_cmp != PartialOrder::NC) {
            return high_cmp;
        }
        return low_cmp;
    }

    /**
     * Instantiate and initialize the combiner from the metadata.
     */
    static PartialOrder *create(Metadata *m /**< [in] Metadata containing configuration setttings for the partial order. */) {
        if (!m) {
            HAGGLE_ERR("Could not initialize combiner. missing metadata.\n");
            return NULL;
        }
        string high_class_name = m->getParameter("high_class_name");
        if (high_class_name == "") {
            HAGGLE_ERR("Missing `high_class_name` parameter.\n");
            return NULL;
        }
        string high_class_param_name = m->getParameter("high_class_param_name");
        if (high_class_param_name == "") {
            HAGGLE_ERR("Missing `high_class_param_name` parameter.\n");
            return NULL;
        }
        Metadata *po_m = m->getMetadata(high_class_param_name);
        PartialOrder *high_po = PartialOrderFactory::getPartialOrderForConfig(high_class_name, po_m);

        if (!high_po) {
            HAGGLE_ERR("Could not instantiate high class.\n");
            return NULL;
        }

        string low_class_name = m->getParameter("low_class_name");
        if (low_class_name == "") {
            HAGGLE_ERR("Missing `low_class_name` parameter.\n");
            return NULL;
        }

        string low_class_param_name = m->getParameter("low_class_param_name");
        if (high_class_param_name == "") {
            HAGGLE_ERR("Missing `low_class_param_name` parameter.\n");
            return NULL;
        }

        po_m = m->getMetadata(low_class_param_name);
        PartialOrder *low_po = PartialOrderFactory::getPartialOrderForConfig(low_class_name, po_m);
        if (!low_po) {
            HAGGLE_ERR("Could not instantiate low class.\n");
            return NULL;
        }
        HAGGLE_DBG("Loaded partial order combiner ordered, low class name: %s, high class name: %s.\n", high_class_name.c_str(), low_class_name.c_str());

        return new PartialOrderCombinerOrdered(high_po, low_po);
    }
};

/**
 * A partial order based on a priority attribute, where greater values of
 * the attribute are > in the partial order (they will be sent first).
 * This class is useful for quickly sending high priority data objects.
 */
class PartialOrderPriorityAttribute : public PartialOrder
{
private:
    string priorityAttributeName; /**< The attribute name whose value is interpreted as a priority. */
public:
    /**
     * Construct an Partial Order Priority Attribute given an attribute name.
     */
    PartialOrderPriorityAttribute(string _priorityAttributeName /**< [in] The name of the attribute whose value is interpreted as a priority. */);

    /**
     * Deconstruct a Partial Order Priority Attribute.
     */
    ~PartialOrderPriorityAttribute();

    /**
     * Compare two partial order elements based on the priority attribute value..
     */
    int compare(PartialOrderElement *a /**< [in] The first element to the binary relation. */, PartialOrderElement *b /**< [in] The second element to the binary relation. */) {
        if (isInf(a)) {
            return PartialOrder::GT;
        }
        if (isInf(b)) {
            return PartialOrder::LT;
        }
        if (!a || !b) {
            HAGGLE_ERR("Data object missing.\n");
            return PartialOrder::NC;
        }
        List<SendPriorityElement *> a_equivs;
        a->getEquivObjects(&a_equivs);

        if (a_equivs.size() == 0) {
            HAGGLE_ERR("Missing element a equivs\n");
            return PartialOrder::NC;
        }

        SendPriorityElement *a_se = a_equivs.front();
        DataObjectRef a_do = a_se->getDataObject();

        if (!a_do) {
            HAGGLE_ERR("Missing element a data object\n");
            return PartialOrder::NC;
        }

        List<SendPriorityElement *> b_equivs;
        b->getEquivObjects(&b_equivs);

        SendPriorityElement *b_se = b_equivs.front();
        DataObjectRef b_do = b_se->getDataObject();

        if (!b_do) {
            HAGGLE_ERR("Missing element b data object\n");
            return PartialOrder::NC;
        }

        const Attribute *a_attr = a_do->getAttribute(priorityAttributeName);
        if (!a_attr) {
            return PartialOrder::NC;
        }

        const Attribute *b_attr = b_do->getAttribute(priorityAttributeName);
        if (!b_attr) {
            return PartialOrder::NC;
        }
        string a_value_str = a_attr->getValue();
        int a_value = atoi(a_value_str.c_str());
        string b_value_str = b_attr->getValue();
        int b_value = atoi(b_value_str.c_str());
        if (a_value == b_value) {
            return PartialOrder::EQ;
        }
        if (a_value < b_value) {
            return PartialOrder::LT;
        }
        return PartialOrder::GT;
    }

    /**
     * Factory method to construct the partial order from metadata extracted from the configuration file.
     * @return The constructed Priority Partial Order initialized with the passed configuration settings.
     */
    static PartialOrder *create(Metadata *m /**< [in] Metadata containing configuration setttings for the partial order. */) {
        if (!m) {
            HAGGLE_ERR("Missing metadata\n");
            return NULL;
        }
        string priority_attribute_name = m->getParameter("priority_attribute_name");
        if (priority_attribute_name != "") {
            PartialOrderPriorityAttribute *po = new PartialOrderPriorityAttribute(priority_attribute_name);
            HAGGLE_DBG("Loaded Partial Order Priority Attribute class, attribute name: %s\n", priority_attribute_name.c_str());
            return po;
        }

        HAGGLE_ERR("Missing required priority_attribute_name parameter\n");
        return NULL;
    }
};

/**
 * A partial order based on an attribute, where data objects that have the
 * the attribute are > than data objects without the attribute in the partial
 * order. Data objects that both have it or do not have it are incomparable.
 */
class PartialOrderAttribute : public PartialOrder
{
private:
    string attributeName; /**< The attribute that is given priority. */
public:
    /**
     * Construct an Partial Order Attribute given an attribute name.
     */
    PartialOrderAttribute(string _attributeName /**< [in] The name of the attribute that is given priority. */);

    /**
     * Deconstruct a Partial Order Attribute.
     */
    ~PartialOrderAttribute();

    /**
     * Compare two partial order elements based on the attribute.
     */
    int compare(PartialOrderElement *a /**< [in] The first element to the binary relation. */, PartialOrderElement *b /**< [in] The second element to the binary relation. */) {
        if (isInf(a)) {
            return PartialOrder::GT;
        }
        if (isInf(b)) {
            return PartialOrder::LT;
        }
        if (!a || !b) {
            HAGGLE_ERR("Data object missing.\n");
            return PartialOrder::NC;
        }
        List<SendPriorityElement *> a_equivs;
        a->getEquivObjects(&a_equivs);

        if (a_equivs.size() == 0) {
            HAGGLE_ERR("Missing element a equivs\n");
            return PartialOrder::NC;
        }

        SendPriorityElement *a_se = a_equivs.front();
        DataObjectRef a_do = a_se->getDataObject();

        if (!a_do) {
            HAGGLE_ERR("Missing element a data object\n");
            return PartialOrder::NC;
        }

        List<SendPriorityElement *> b_equivs;
        b->getEquivObjects(&b_equivs);

        SendPriorityElement *b_se = b_equivs.front();
        DataObjectRef b_do = b_se->getDataObject();

        if (!b_do) {
            HAGGLE_ERR("Missing element b data object\n");
            return PartialOrder::NC;
        }

        const Attribute *a_attr = a_do->getAttribute(attributeName);
        const Attribute *b_attr = b_do->getAttribute(attributeName);
        if (!a_attr && b_attr) {
            return PartialOrder::LT;
        }

        if (a_attr && !b_attr) {
            return PartialOrder::GT;
        }

        return PartialOrder::NC;
    }

    /**
     * Factory method to construct the partial order from metadata extracted from the configuration file.
     * @return The constructed Priority Partial Order initialized with the passed configuration settings.
     */
    static PartialOrder *create(Metadata *m /**< [in] Metadata containing configuration setttings for the partial order. */) {
        if (!m) {
            HAGGLE_ERR("Missing metadata\n");
            return NULL;
        }
        string attribute_name = m->getParameter("attribute_name");
        if (attribute_name != "") {
            PartialOrderAttribute *po = new PartialOrderAttribute(attribute_name);
            HAGGLE_DBG("Loaded Partial Order Attribute class, attribute name: %s\n", attribute_name.c_str());
            return po;
        }

        HAGGLE_ERR("Missing required attribute_name parameter\n");
        return NULL;
    }
};

/**
 * Constructor for partial order attribute.
 */
PartialOrderAttribute::PartialOrderAttribute(string _attributeName) :
    attributeName(_attributeName)
{
}

/**
 * Deconstructor for partial order attribute.
 */
PartialOrderAttribute::~PartialOrderAttribute()
{
}

/**
 * Instantiate and initialize a partial order from its class name
 * and metadata.
 * @return The initialized partial order.
 */
PartialOrder *
PartialOrderFactory::getPartialOrderForConfig(
        string name /**< [in] The name of the partial order class to load. */,
        Metadata *m /**< [in] Metadata containing configuration setttings for the partial order. */) {

    PartialOrder *po = NULL;
    if (strcmp(name.c_str(), "PartialOrderPriorityAttribute") == 0) {
        po = PartialOrderPriorityAttribute::create(m);
    }
    else if (strcmp(name.c_str(), "PartialOrderAttribute") == 0) {
        po = PartialOrderAttribute::create(m);
    }
    else if (strcmp(name.c_str(), "PartialOrderCombinerOrdered") == 0) {
        po = PartialOrderCombinerOrdered::create(m);
    }
    else if (strcmp(name.c_str(), "PartialOrderFIFO") == 0) {
        po = PartialOrderFIFO::create(m);
    }
    else {
        HAGGLE_ERR("Unknown class name for partial order: %s\n", name.c_str());
    }

    if (!po) {
        HAGGLE_ERR("Could not instantiate partial order: %s\n", name.c_str());
    }
    return po;
}

/**
 * Construct a PartialOrderPriorityAttribute.
 */
PartialOrderPriorityAttribute::PartialOrderPriorityAttribute(string _priorityAttributeName)
    : priorityAttributeName(_priorityAttributeName) {}

/**
 * Deconstruct the PartialOrderPriorityAttribute.
 */
PartialOrderPriorityAttribute::~PartialOrderPriorityAttribute() {};

/**
 * Constructs a topological order given a partial order and inserted elements.
 * Effectively a partial order heap, using a Hesse Graph.
 */
class TopologicalSorter {
private:
    PartialOrder *partialOrder; /**< The partial order used to sort the elements. */
    PartialOrderElement *root; /**< The root element of the heap (Inf in the partial order). */
    int numElements; /**< The number of elements in the topology sort. */
    int maxElements; /**< The maximum number of elements inserted. */
public:
    /**
     * Construct a new topological sorter given a partial order.
     */
    TopologicalSorter(PartialOrder *_partialOrder /**< [in] The partial order used to order the heap. */) :
        partialOrder(_partialOrder),
        root(new PartialOrderElement((SendPriorityElement *)NULL)),
        numElements(0),
        maxElements(0) {};

    /**
     * Free the resources used by the topological sorter.
     */
    ~TopologicalSorter() {
        HAGGLE_DBG("Maximum queue size in top sorter: %d\n", getMaxSize());
        SendPriorityElement *top = NULL;
        while ((top = popTop()))
        {
            delete top;
        }
        if (partialOrder) {
            delete partialOrder;
        }
        // CBMEN, HL - Fix memory leak
        if (root) {
            delete root;
        }
    }

    /**
     * Add an element to the heap, to be sorted.
     * The idea is to push the element as little possible towards the bottom of
     * the graph, respecting the ordering. Inf is the top node in the graph.
     */
    void addElement(SendPriorityElement *se /**< [in] The element to be inserted and sorted. */) {
        if (!se) {
            HAGGLE_ERR("Cannot add NULL data object.\n");
            return;
        }

        PartialOrderElement *po_se = new PartialOrderElement(se);

        List<Pair<PartialOrderElement *, PartialOrderElement *> > strands;
        bool hasComparable = false;
        List<PartialOrderElement *>children;
        root->getChildren(&children);
        // check if there are any children of the root that the inserted element is comparable to
        for (List<PartialOrderElement *>::iterator it = children.begin(); it != children.end(); it++) {
            if (partialOrder->compare(po_se, *it) != PartialOrder::NC) {
                strands.push_back(make_pair(*it, root));
                hasComparable = true;
            }
        }
        if (!hasComparable) {
            // no comparables, so add as a child of the root
            root->addChild(po_se);
            numElements++;
            if (numElements > maxElements) {
                maxElements = numElements;
            }
            return;
        }
        // otherwise, iterate across comparables, looking for
        while (strands.size() > 0) {
            Pair<PartialOrderElement *, PartialOrderElement *> top = strands.front();
            strands.pop_front();
            PartialOrderElement *child = top.first;
            PartialOrderElement *parent = top.second;
            int compare = partialOrder->compare(po_se, child);
            if (compare == PartialOrder::NC) {

            }
            else if (compare == PartialOrder::EQ) {
                List<SendPriorityElement *> equivs;
                po_se->getEquivObjects(&equivs);
                child->addEquivalents(&equivs);
            }
            else if (compare == PartialOrder::LT) {
                if (child->getNumChildren() == 0) {
                    child->addChild(po_se);
                }
                else {
                    bool hasComparable = false;
                    List<PartialOrderElement *> childChild;
                    child->getChildren(&childChild);
                    for (List<PartialOrderElement *>::iterator it = childChild.begin(); it != childChild.end(); it++) {
                        if (partialOrder->compare(po_se, *it) != PartialOrder::NC) {
                            strands.push_back(make_pair(*it, child));
                            hasComparable = true;
                        }
                    }
                    if (!hasComparable) {
                        child->addChild(po_se);
                    }
                }
            }
            else if (compare == PartialOrder::GT) {
                parent->removeChild(child);
                po_se->addChild(child);
                parent->addChild(po_se);
            }
        }
        numElements++;
        if (numElements > maxElements) {
            maxElements = numElements;
        }
    }

    /**
     * Take the element with highest priority off the heap (provides
     * a topological sort of the partial order).
     * @return The top element from the heap.
     */
    SendPriorityElement *popTop() {
        SendPriorityElement *top = TopologicalSorter::popTop(root);
        if (top) {
            numElements--;
        }
        return top;
    }

    /**
     * Take the element with highest priority off the heap (provides
     * a topological sort of the partial order).
     * Accepts an optional root to pop from.
     * @return The top element from the heap.
     */
    static
    SendPriorityElement *popTop(PartialOrderElement *opt_root /**< [in] The root element to start finding the top element from. */)
    {
        if (!opt_root) {
            HAGGLE_ERR("Missing top partial order element.\n");
            return NULL;
        }
        if (opt_root->getNumChildren() == 0) {
            HAGGLE_DBG("No children on root.\n");
            return NULL;
        }
        List<PartialOrderElement *> children;
        opt_root->getChildren(&children);
        PartialOrderElement *topChild = children.front();
        if (!topChild) {
            HAGGLE_ERR("NULL child\n");
            return NULL;
        }

        SendPriorityElement *topChildSE = topChild->popEquivalent();
        if (!topChildSE) {
            HAGGLE_ERR("NULL top child send priority element\n");
            return NULL;
        }

        if (topChild->getNumEquivalent() == 0) {
            // we popped the last equivalent, move children over
            List<PartialOrderElement *> topChildren;
            topChild->getChildren(&topChildren);
            opt_root->addChildren(&topChildren);
            opt_root->removeChild(topChild);
            delete topChild;
        }

        return topChildSE;
    }

    /**
     * Get the number of elements in the heap.
     * @return The number of elements in the heap.
     */
    int getSize() {
        return numElements;
    }

    /**
     * Get the maximum number of elements at one time in the queue.
     * @return The maximum number of elements at one time in the queue.
     */
    int getMaxSize() {
        return maxElements;
    }

    /**
     * Print all of the elements in the queue to haggle debug.
     */
    void printDebug() {
        PartialOrderElement *rootClone = root->makeCopyWithoutParent();
        SendPriorityElement *se = NULL;
        HAGGLE_DBG("QUEUE: Printing queue (%d) contents:\n", getSize());
        while ((se = popTop(rootClone))) {
            HAGGLE_DBG("QUEUE: Element: %s, %d\n", DataObject::idString(se->getDataObject()).c_str(), se->getDataObject()->getOrigDataLen());
        }
        HAGGLE_DBG("QUEUE: Done printing queue.\n");
    }
};

/**
 * Construct a new send queue.
 */
SendQueue::SendQueue(
    HaggleKernel *_kernel, /**< The haggle kernel used to post events. */
    string _partialOrderName, /**< The name of the partial order used to order the queues. */
    Metadata *_partialOrderMetdata, /**< The metadata used to initialize the partial order. */
    int _parallelFactor /**< The max number of concurrent data object transmissions. */)
    :   kernel(_kernel),
        partialOrderName(_partialOrderName),
        partialOrderMetdata(_partialOrderMetdata ? _partialOrderMetdata->copy() : NULL),
        parallelFactor(_parallelFactor)
{
}

/**
 * Deconstruct a send queue.
 */
SendQueue::~SendQueue()
{
    if (partialOrderMetdata) {
        delete partialOrderMetdata;
    }

    for(senders_t::iterator it = senders.begin(); it != senders.end(); it++) {
        send_state_t *sendState = (*it).second;
        if (!sendState) {
            HAGGLE_ERR("NULL send state.\n");
            continue; // CBMEN, HL
        }
        TopologicalSorter *topSorter = sendState->topSorter;
        if (!topSorter) {
            HAGGLE_ERR("NULL top sorter.\n");
            continue; // CBMEN, HL
        }
        //senders.erase(it); CBMEN, HL - Don't delete while iterating
        delete topSorter;
        free(sendState);
    }
}

/**
 * Queue and send data objects for a send state.
 */
void
SendQueue::startSend(send_state_t *sendState /**< The send state whose data objects will be sent. */)
{
    if (!sendState) {
        HAGGLE_ERR("Missing parameters\n");
        return;
    }

    TopologicalSorter *topologicalSorter = sendState->topSorter;
    if (!topologicalSorter) {
        HAGGLE_ERR("Topological sorter NULL.\n");
        return;
    }

    SendPriorityElement *se = topologicalSorter->popTop();
    if (!se) {
        HAGGLE_DBG("Queue is empty.\n");
        return;
    }

    DataObjectRef dObj = se->getDataObject();
    if (!dObj) {
        HAGGLE_ERR("NULL data object.\n");
        delete se;
        return;
    }

    NodeRef node = se->getNode();
    if (!node) {
        HAGGLE_ERR("NULL node\n");
        delete se;
        return;
    }

    EventType sendCallback = se->getSendCallback();
    if (!sendCallback) {
        HAGGLE_ERR("No send callback.\n");
        return;
    }

    NodeRefList nodeRefSingleton;
    nodeRefSingleton.push_back(node);

    if (sendState->numActiveSendStations < parallelFactor) {
        sendState->sendStations->push_back(se);
        sendState->numActiveSendStations++;
        kernel->addEvent(new Event(sendCallback, dObj, nodeRefSingleton));
    }
    else {
        // put it back
        topologicalSorter->addElement(se);
    }
}

/**
 * Add a send priority element to be sent.
 */
void
SendQueue::addSendPriorityElement(SendPriorityElement *se)
{
    if (!se) {
        HAGGLE_ERR("Missing parameters\n");
        return;
    }

    DataObjectRef dObj = se->getDataObject();
    if (!dObj) {
        HAGGLE_ERR("Missing data object.\n");
        return;
    }

    NodeRef node = se->getNode();
    if (!node) {
        HAGGLE_ERR("Missing node.\n");
        return;
    }

    send_state_t *sendState = getSendState(dObj, node);
    if (!sendState) {
        sendState = createSendState(dObj, node);
    }
    if (!sendState) {
        HAGGLE_ERR("Could not create send state.\n");
        return;
    }

    TopologicalSorter *topologicalSorter = sendState->topSorter;
    if (!topologicalSorter) {
        HAGGLE_ERR("Topological sorter NULL.\n");
        return;
    }

    topologicalSorter->addElement(se);
    startSend(sendState);

    if (sendState->numActiveSendStations == 0) {
        // queues are drained, removing state
        HAGGLE_ERR("Somehow send queue is empty even thouhg added element.\n");
        removeSendState(dObj, node);
    }
}

/**
 * Remove a send priority element from the send queue.
 * @return true if the element was successfully removed, false otherwise.
 */
bool
SendQueue::removeSendPriorityElement(SendPriorityElement *se)
{
    if (!se) {
        HAGGLE_ERR("NULL send priority element.\n");
        return false;
    }

    DataObjectRef dObj = se->getDataObject();
    if (!dObj) {
        HAGGLE_ERR("Missing data object.\n");
        return false;
    }

    NodeRef node = se->getNode();
    if (!node) {
        HAGGLE_ERR("Missing node.\n");
        return false;
    }

    send_state_t *sendState = getSendState(dObj, node);
    if (!sendState) {
        HAGGLE_DBG("Missing send state.\n");
        return false;
    }

    bool foundElement = false;
    for (send_stations_t::iterator it = sendState->sendStations->begin(); it != sendState->sendStations->end(); it++) {
        SendPriorityElement *se = (*it);
        if (!se) {
            HAGGLE_ERR("NULL element\n");
            break;
        }
        if (dObj == se->getDataObject() && node == se->getNode()) {
            foundElement = true;
            sendState->sendStations->erase(it);
            Timeval now = Timeval::now();
            Timeval createTime = se->getCreateTime();
            HAGGLE_DBG("Duration in send queue: %d\n", (int)(now-createTime).getTimeAsMilliSeconds());
            delete se;
            sendState->numActiveSendStations--;
            break;
        }
    }

    if (!foundElement) {
        HAGGLE_DBG("Element missing from send station.\n");
    }

    startSend(sendState);

    if (sendState->numActiveSendStations == 0) {
        // queues are drained, removing state
        removeSendState(dObj, node);
    }

    return foundElement;
}

/**
 * Create send state responsible for the <data object, node> pair.
 * @return The send state responsible for the <data object, node> pair.
 */
send_state_t *
SendQueue::createSendState(
    DataObjectRef dObj, /**< The data object whose send state is created. */
    NodeRef node /**< The node whose send state is created. */)
{
    if (!dObj || !node) {
        HAGGLE_ERR("Missing params.\n");
        return NULL;
    }

    senders_t::iterator it = senders.find(Node::nameString(node));
    if (it != senders.end()) {
        HAGGLE_ERR("Send state already constructed.\n");
        return NULL;
    }

    send_state_t *sendState = (send_state_t *)malloc(sizeof(send_state_t));
    bzero(sendState, sizeof(send_state_t));
    if (!sendState) {
        HAGGLE_ERR("Could not malloc send state.\n");
        return NULL;
    }

    PartialOrder *newPartialOrder = PartialOrderFactory::getPartialOrderForConfig(partialOrderName, partialOrderMetdata);
    if (!newPartialOrder) {
        HAGGLE_ERR("Could not construct partial order.\n");
        return NULL;
    }

    TopologicalSorter *newTopSorter = new TopologicalSorter(newPartialOrder);
    if (!newTopSorter) {
        HAGGLE_ERR("Could not construct topological sorter.\n");
        return NULL;
    }

    List<SendPriorityElement *> *sendStations = new List<SendPriorityElement *>();
    if (!sendStations) {
        HAGGLE_ERR("Could not construct send stations.\n");
        return NULL;
    }

    sendState->topSorter = newTopSorter;
    sendState->sendStations = sendStations;
    sendState->numActiveSendStations = 0;

    senders.insert(make_pair(Node::nameString(node), sendState));

    return sendState;
}

/**
 * Remove send state responsible for the <data object, node> pair.
 */
void
SendQueue::removeSendState(
    DataObjectRef dObj, /**< The data object whose state is being removed. */
    NodeRef node /**< The node whose state is being removed. */)
{
    //if (!dObj || !node) {
    if (!node) {
        HAGGLE_ERR("Missing params.\n");
        return;
    }

    senders_t::iterator it = senders.find(Node::nameString(node));
    if (it == senders.end()) {
        HAGGLE_ERR("Send state missing.\n");
        return;
    }

    send_state_t *sendState = (*it).second;
    if (!sendState) {
        HAGGLE_ERR("NULL send state.\n");
        return;
    }

    TopologicalSorter *topSorter = sendState->topSorter;
    if (!topSorter) {
        HAGGLE_ERR("NULL top sorter.\n");
        return;
    }

    send_stations_t *sendStations = sendState->sendStations;
    if (!sendStations) {
        HAGGLE_ERR("NULL send stations.\n");
        return;
    }

    senders.erase(it);

    delete topSorter;
    delete sendStations;
    free(sendState);
}

/**
 * Get the send state responsible for the <data object, node> pair.
 * @return The send state responsible for the <data object, node> pair.
 */
send_state_t *
SendQueue::getSendState(
    DataObjectRef dObj, /**< The data object whose queue state is fetched. */
    NodeRef node /**< The node whose queue state is fetched. */)
{
    //if (!dObj || !node) {
    if (!node) {
        HAGGLE_ERR("Missing params.\n");
        return NULL;
    }

    senders_t::iterator it = senders.find(Node::nameString(node));
    if (it == senders.end()) {
        return NULL;
    }

    send_state_t *sendState = (*it).second;
    if (!sendState) {
        HAGGLE_ERR("NULL send state.\n");
        return NULL;
    }

    return sendState;
}

/**
 * Construct a SendPriorityManager.
 */
SendPriorityManager::SendPriorityManager(HaggleKernel *_haggle) :
    Manager("SendPriorityManager", _haggle),
    enabled(false),
    debug(false),
    numSendSuccess(0),
    numSendFailure(0),
    sendQueue(new SendQueue(_haggle, "PartialOrderFIFO", NULL, 1))
{
}

/**
 * Deconstruct the SendPriorityManager.
 */
SendPriorityManager::~SendPriorityManager()
{
    if (sendQueue) {
        delete sendQueue;
    }
}

/**
 * Initialize a new SendPriorityManager during start-up.
 * @return True if initializion was successful, false otherwise.
 */
bool
SendPriorityManager::init_derived()
{
#define __CLASS__ SendPriorityManager
    int ret;

    ret = setEventHandler(EVENT_TYPE_SEND_PRIORITY_SUCCESS, onSendSuccessful);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_SEND_PRIORITY_FAILURE, onSendFailure);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_SEND_PRIORITY, onSendPriority);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    return true;
}

/**
 * Configuration config.xml handler during start-up.
 */
void
SendPriorityManager::onConfig(
    Metadata *m)
{
    const char *param = m->getParameter("enable");

    if (param)  {
        if (strcmp(param, "true") == 0) {
            enabled = true;
        }
        else if (strcmp(param, "false") == 0) {
            enabled = false;
        }
        else {
            HAGGLE_ERR("Enabled param must be true or false\n");
            return;
        }
    }

    if (!enabled) {
        HAGGLE_DBG("SendPriorityManager disabled.\n");
        return;
    }

    param = m->getParameter("debug");

    if (param)  {
        if (strcmp(param, "true") == 0) {
            debug = true;
        }
        else if (strcmp(param, "false") == 0) {
            debug = false;
        }
        else {
            HAGGLE_ERR("debug param must be true or false\n");
            return;
        }
    }

    int parallelFactor = 1;
    if ((param = m->getParameter("parallel_factor"))) {
        char *endptr = NULL;
        unsigned long tmp = strtoul(param, &endptr, 10);
        if (endptr && endptr != param) {
            parallelFactor = tmp;
        }
    }

    param = m->getParameter("run_self_tests");
    bool runSelfTests = false;
    if (param)  {
        if (strcmp(param, "true") == 0) {
            runSelfTests = true;
        }
        else if (strcmp(param, "false") == 0) {
            runSelfTests = false;
        }
        else {
            HAGGLE_ERR("run_self_tests param must be true or false\n");
            return;
        }
    }

    param = m->getParameter("partial_order_class");
    if (param) {
        if (sendQueue) {
            delete sendQueue;
        }
        sendQueue = new SendQueue(kernel, param, m->getMetadata(param), parallelFactor);
    }
    else {
        HAGGLE_DBG2("No partial order specified.\n");
    }

    if (runSelfTests) {
        if (selfTests()) {
            HAGGLE_STAT("Summary Statistics - %s - Unit tests PASSED\n", getName());
        }
        else {
            HAGGLE_STAT("Summary Statistics - %s - Unit tests FAILED\n", getName());
        }
    }

    HAGGLE_DBG("Loaded send priority manager, run self tests: %s, enabled: %s, debug: %s, parallel factor: %d\n", runSelfTests ? "yes" : "no", enabled ? "yes" : "no", debug ? "yes" : "no", parallelFactor);
}

/**
 * Handler for a send success. Removes the DO that was sent from the send
 * queue and queues up another DO to send.
 */
void
SendPriorityManager::onSendSuccessful(
    Event *e)
{
    if (!enabled) {
        return;
    }

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring new interest\n");
        return;
    }

    if (!e || !e->hasData()) {
        HAGGLE_ERR("Missing data\n");
        return;
    }

    SendPriorityElement *se = (SendPriorityElement *)e->getData();

    if (sendQueue->removeSendPriorityElement(se)) {
        numSendSuccess++;
    }
    else {
        HAGGLE_DBG("Element missing from send station.\n");
    }

    // CBMEN, HL - fix memory leak
    if (se) {
        delete se;
    }
}

/**
 * Handler for a send failure. Removes the DO to be sent from the send
 * queues and queues up another DO to send.
 */
void
SendPriorityManager::onSendFailure(
    Event *e)
{
    if (!enabled) {
        return;
    }

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring new interest\n");
        return;
    }

    if (!e || !e->hasData()) {
        HAGGLE_ERR("Missing data\n");
        return;
    }

    SendPriorityElement *se = (SendPriorityElement *)e->getData();

    if (sendQueue->removeSendPriorityElement(se)) {
        numSendFailure++;
    }
    else {
        HAGGLE_DBG("Element missing from send station.\n");
    }

    // CBMEN, HL - fix memory leak
    if (se) {
        delete se;
    }
}


/**
 * Handler for the EVENT_TYPE_SEND_PRIORITY event raised by the ProtocolManager
 * to send a data object.  Places the data object on the send queues which
 * are ordered by priority.
 */
void
SendPriorityManager::onSendPriority(
    Event *e)
{
    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring new interest\n");
        return;
    }

    if (!e || !e->hasData()) {
        HAGGLE_ERR("Missing data\n");
        return;
    }


    SendPriorityElement *se = (SendPriorityElement *)e->getData();

    if (!se) {
        HAGGLE_ERR("Missing send element.\n");
        return;
    }

    if (enabled) {
        sendQueue->addSendPriorityElement(se);
        return;
    }

    EventType sendCallback = se->getSendCallback();
    if (!sendCallback) {
        HAGGLE_ERR("No send callback.\n");
        return;
    }

    DataObjectRef dObj = se->getDataObject();
    if (!dObj) {
        HAGGLE_ERR("NULL data object.\n");
        delete se;
        return;
    }

    NodeRef node = se->getNode();
    if (!node) {
        HAGGLE_ERR("NULL node\n");
        delete se;
        return;
    }

    NodeRefList nodeRefSingleton;
    nodeRefSingleton.push_back(node);

    kernel->addEvent(new Event(sendCallback, dObj, nodeRefSingleton));
}

/**
 * Verifies that the Partial Order prioritization works correctly.
 * Data objects with higher prioritization are sent before data objects
 * with lower prioritization.
 * @return True if the Partial Order prioritization unit test passes,
 *  False otherwise.
 */
bool
SendPriorityManager::selfTest1()
{
    // unit test for partial order priority
    PartialOrder *partialOrder = new PartialOrderPriorityAttribute("priority");
    TopologicalSorter *topSorter = new TopologicalSorter(partialOrder);

    bool stat = true;

    // insert counting up

    for (int i = 0; i < 10; i++) {
        DataObjectRef fakeDO = DataObject::create();
        string prior;
        stringprintf(prior, "%d", i);
        fakeDO->addAttribute("priority", prior.c_str());
        SendPriorityElement *fakeSE = new SendPriorityElement(fakeDO, NULL);
        topSorter->addElement(fakeSE);
    }

    int latestMax = 11;
    for (int i = 0; i < 10; i++) {
        SendPriorityElement *top = topSorter->popTop();
        if (!top) {
            HAGGLE_ERR("Could not pop top\n");
            stat = false;
            break;
        }
        DataObjectRef fakeDO = top->getDataObject();
        delete top;
        if (!fakeDO) {
            HAGGLE_ERR("Top DO is NULL\n");
            stat = false;
            break;
        }
        const Attribute *attr = fakeDO->getAttribute("priority");
        if (!attr) {
            HAGGLE_ERR("Missing priority attribute.\n");
            stat = false;
            break;
        }
        string value = attr->getValue();
        int prior = atoi(value.c_str());
        if (prior > latestMax) {
            HAGGLE_ERR("Self test 1 count up: Sort failed.\n");
            topSorter->printDebug();
            stat = false;
            break;
        }
        latestMax = prior;
    }

    if (!stat) {
        return false;
    }

    stat = true;

    // insert counting down

    for (int i = 10; i > 0; i--) {
        DataObjectRef fakeDO = DataObject::create();
        string prior;
        stringprintf(prior, "%d", i);
        fakeDO->addAttribute("priority", prior.c_str());
        SendPriorityElement *fakeSE = new SendPriorityElement(fakeDO, NULL);
        topSorter->addElement(fakeSE);
    }

    latestMax = 11;
    for (int i = 0; i < 10; i++) {
        SendPriorityElement *top = topSorter->popTop();
        if (!top) {
            HAGGLE_ERR("Could not pop top\n");
            stat = false;
            break;
        }
        DataObjectRef fakeDO = top->getDataObject();
        delete top;
        if (!fakeDO) {
            HAGGLE_ERR("Top DO is NULL\n");
            stat = false;
            break;
        }
        const Attribute *attr = fakeDO->getAttribute("priority");
        if (!attr) {
            HAGGLE_ERR("Missing priority attribute.\n");
            stat = false;
            break;
        }
        string value = attr->getValue();
        int prior = atoi(value.c_str());
        if (prior > latestMax) {
            HAGGLE_ERR("Self test 1 count down: Sort failed.\n");
            topSorter->printDebug();
            stat = false;
            break;
        }
        latestMax = prior;
    }

    if (!stat) {
        return false;
    }

    stat = true;

    delete topSorter;

    return true;
}

/**
 * Verifies that the default FIFO prioritization works correctly.
 * Data objects will be sent in order of their insertion into the send queue.
 * @return True if the FIFO prioritization unit tests passes, False otherwise.
 */
bool
SendPriorityManager::selfTest2()
{
    // unit test for FIFO priority
    PartialOrder *partialOrder = new PartialOrderFIFO();
    TopologicalSorter *topSorter = new TopologicalSorter(partialOrder);

    for (int i = 9; i >= 0; i--) {
        DataObjectRef fakeDO = DataObject::create();
        string prior;
        stringprintf(prior, "%d", i);
        fakeDO->addAttribute("priority", prior.c_str());
        SendPriorityElement *fakeSE = new SendPriorityElement(fakeDO, NULL);
        topSorter->addElement(fakeSE);
    }

    bool stat = true;

    int latestMax = 11;
    for (int i = 0; i < 10; i++) {
        SendPriorityElement *top = topSorter->popTop();
        if (!top) {
            HAGGLE_ERR("Could not pop top: i = %d\n", i);
            stat = false;
            break;
        }
        DataObjectRef fakeDO = top->getDataObject();
        delete top;
        if (!fakeDO) {
            HAGGLE_ERR("Top DO is NULL\n");
            stat = false;
            break;
        }
        const Attribute *attr = fakeDO->getAttribute("priority");
        if (!attr) {
            HAGGLE_ERR("Missing priority attribute.\n");
            stat = false;
            break;
        }
        string value = attr->getValue();
        int prior = atoi(value.c_str());
        if (prior > latestMax) {
            HAGGLE_ERR("Self test 2: Sort failed: prior: %d, latestMax: %d.\n", prior, latestMax);
            topSorter->printDebug();
            stat = false;
            break;
        }
        latestMax = prior;
    }

    delete topSorter;

    return stat;
}

/**
 * Verifies that the ordered combiner partial order works correctly.
 * @return True if the ordered combiner unit tests passes, False otherwise.
 */
bool
SendPriorityManager::selfTest3()
{
    PartialOrder *highPartialOrder = new PartialOrderPriorityAttribute("priority");
    PartialOrder *loPartialOrder = new PartialOrderFIFO();
    PartialOrder *combinerPartialOrder = new PartialOrderCombinerOrdered(highPartialOrder, loPartialOrder);

    TopologicalSorter *topSorter = new TopologicalSorter(combinerPartialOrder);

    for (int i = 9; i >= 0; i--) {
        DataObjectRef fakeDO = DataObject::create();
        string prior;
        stringprintf(prior, "%d", i);
        if (i % 2 == 0) {
            // remove priority attribute for every other, falling back on FIFO
            fakeDO->addAttribute("priority", prior.c_str());
        }
        fakeDO->addAttribute("print", prior.c_str());
        SendPriorityElement *fakeSE = new SendPriorityElement(fakeDO, NULL);
        topSorter->addElement(fakeSE);
    }

    bool stat = true;

    int latestMax = 11;
    for (int i = 0; i < 10; i++) {
        SendPriorityElement *top = topSorter->popTop();
        if (!top) {
            HAGGLE_ERR("Could not pop top: i = %d\n", i);
            stat = false;
            break;
        }
        DataObjectRef fakeDO = top->getDataObject();
        delete top;
        if (!fakeDO) {
            HAGGLE_ERR("Top DO is NULL\n");
            stat = false;
            break;
        }
        const Attribute *attr = fakeDO->getAttribute("print");
        if (!attr) {
            HAGGLE_ERR("Missing priority attribute.\n");
            stat = false;
            break;
        }
        string value = attr->getValue();
        int prior = atoi(value.c_str());
        if (prior > latestMax) {
            HAGGLE_ERR("Self test 3: Sort failed: prior: %d, latestMax: %d.\n", prior, latestMax);
            topSorter->printDebug();
            stat = false;
            break;
        }
        latestMax = prior;
    }

    delete topSorter;

    return stat;
}

/**
 * Verifies that the attribute partial order works correctly.
 * @return True if the ordered combiner unit tests passes, False otherwise.
 */
bool
SendPriorityManager::selfTest4()
{
    PartialOrder *partialOrder = new PartialOrderAttribute("bolo");
    TopologicalSorter *topSorter = new TopologicalSorter(partialOrder);

    int num_bolos = 0;
    for (int i = 9; i >= 0; i--) {
        DataObjectRef fakeDO = DataObject::create();
        if (i % 2 == 0) {
            // remove priority attribute for every other, falling back on FIFO
            fakeDO->addAttribute("bolo", "");
            num_bolos++;
        }
        SendPriorityElement *fakeSE = new SendPriorityElement(fakeDO, NULL);
        topSorter->addElement(fakeSE);
    }

    bool stat = true;

    int cur_bolos = 0;
    for (int i = 0; i < 10; i++) {
        SendPriorityElement *top = topSorter->popTop();
        if (!top) {
            HAGGLE_ERR("Could not pop top: i = %d\n", i);
            stat = false;
            break;
        }
        DataObjectRef fakeDO = top->getDataObject();
        delete top;
        if (!fakeDO) {
            HAGGLE_ERR("Top DO is NULL\n");
            stat = false;
            break;
        }
        const Attribute *attr = fakeDO->getAttribute("bolo");

        // make sure all the bolos come before the non-bolo data
        if ((cur_bolos < num_bolos) && !attr) {
            HAGGLE_ERR("Self test 4: Sort failed: Missing priority attribute.\n");
            stat = false;
            break;
        }
        else {
            cur_bolos++;
        }
    }

    delete topSorter;

    return stat;
}

/**
 * Verifies that the complex partial order works correctly.
 * @return True if the attribute unit tests passes, False otherwise.
 */
bool
SendPriorityManager::selfTest5()
{
    PartialOrder *partialOrder = new PartialOrderAttribute("bolo");
    PartialOrder *attributeOrder = new PartialOrderPriorityAttribute("priority");
    PartialOrder *fifoOrder = new PartialOrderFIFO();
    PartialOrder *combinerOrder1 = new PartialOrderCombinerOrdered(attributeOrder, fifoOrder);
    PartialOrder *combinerOrder2 = new PartialOrderCombinerOrdered(partialOrder, combinerOrder1);
    TopologicalSorter *topSorter = new TopologicalSorter(combinerOrder2);

    {
        DataObjectRef fakeDO = DataObject::create();
        fakeDO->addAttribute("priority", "1");
        fakeDO->addAttribute("print", "4");
        SendPriorityElement *fakeSE = new SendPriorityElement(fakeDO, NULL);
        topSorter->addElement(fakeSE);
    }

    {
        DataObjectRef fakeDO = DataObject::create();
        fakeDO->addAttribute("priority", "2");
        fakeDO->addAttribute("print", "3");
        SendPriorityElement *fakeSE = new SendPriorityElement(fakeDO, NULL);
        topSorter->addElement(fakeSE);
    }

    {
        DataObjectRef fakeDO = DataObject::create();
        fakeDO->addAttribute("priority", "1");
        fakeDO->addAttribute("bolo", "");
        fakeDO->addAttribute("print", "1");
        SendPriorityElement *fakeSE = new SendPriorityElement(fakeDO, NULL);
        topSorter->addElement(fakeSE);
    }

    {
        DataObjectRef fakeDO = DataObject::create();
        fakeDO->addAttribute("bolo", "");
        fakeDO->addAttribute("print", "2");
        SendPriorityElement *fakeSE = new SendPriorityElement(fakeDO, NULL);
        topSorter->addElement(fakeSE);
    }

    {
        DataObjectRef fakeDO = DataObject::create();
        fakeDO->addAttribute("priority", "2");
        fakeDO->addAttribute("bolo", "");
        fakeDO->addAttribute("print", "0");
        SendPriorityElement *fakeSE = new SendPriorityElement(fakeDO, NULL);
        topSorter->addElement(fakeSE);
    }

    bool stat = true;
    for (int i = 0; i < 5; i++) {
        SendPriorityElement *top = topSorter->popTop();
        if (!top) {
            HAGGLE_ERR("Could not pop top\n");
            stat = false;
            break;
        }
        DataObjectRef fakeDO = top->getDataObject();
        delete top;
        if (!fakeDO) {
            HAGGLE_ERR("Top DO is NULL\n");
            stat = false;
            break;
        }
        const Attribute *attr = fakeDO->getAttribute("print");

        if (!attr) {
            HAGGLE_ERR("Missing print attribute.\n");
            stat = false;
            break;
        }

        string value = attr->getValue();
        int got = atoi(value.c_str());
        if (got != i) {
            HAGGLE_ERR("Got unexpected: got: %d, expected: %d Sort failed.\n", got, i);
            stat = false;
            break;
        }
    }
    return stat;
}

/**
 * Runs all the unit tests if the `run_self_tests` parameter is set to
 * `true` in the configuration file.
 * @return True if all of the tests PASSED, False otherwise.
 */
bool
SendPriorityManager::selfTests()
{
    bool passed = true;
    if (!selfTest1()) {
        passed = false;
        HAGGLE_DBG("self test 1 FAILED.\n");
    }
    else {
        HAGGLE_DBG("self test 1 PASSED.\n");
    }
    if (!selfTest2()) {
        passed = false;
        HAGGLE_DBG("self test 2 FAILED.\n");
    }
    else {
        HAGGLE_DBG("self test 2 PASSED.\n");
    }
    if (!selfTest3()) {
        passed = false;
        HAGGLE_DBG("self test 3 FAILED.\n");
    }
    else {
        HAGGLE_DBG("self test 3 PASSED.\n");
    }
    if (!selfTest4()) {
        passed = false;
        HAGGLE_DBG("self test 4 FAILED.\n");
    }
    else {
        HAGGLE_DBG("self test 4 PASSED.\n");
    }
    if (!selfTest5()) {
        passed = false;
        HAGGLE_DBG("self test 5 FAILED.\n");
    }
    else {
        HAGGLE_DBG("self test 5 PASSED.\n");
    }
    return passed;
}

/**
 * Shutdown handler to clear remaining data structures.
 */
void
SendPriorityManager::onShutdown()
{
    HAGGLE_STAT("SendPriorityManager - num send success: %ld\n", numSendSuccess);
    HAGGLE_STAT("SendPriorityManager - num send failure: %ld\n", numSendFailure);
    unregisterWithKernel();
}

/**
 * Get the event used to queue a data object for sending.
 * @return The event containing information used by the SendPriorityManager
 *  to process the request.
 */
Event *
SendPriorityManager::getSendPriorityEvent(
    DataObjectRef dObj,
    NodeRef target,
    EventType sendPriorityCallback)
{
    if ((target && target->isLocalApplication()) || (dObj && dObj->isControlMessage())) {
        NodeRefList nodeRefSingleton;
        nodeRefSingleton.push_back(target);
        return new Event(sendPriorityCallback, dObj, nodeRefSingleton);
    }
    return new Event(EVENT_TYPE_SEND_PRIORITY, (void *)new SendPriorityElement(dObj, target, sendPriorityCallback));
}

/**
 * Get the event used to dequeue a data object that was sucessfully sent,
 * and queue up another data object to send.
 * @return The event containing information used by the SendPriorityManager
 *  to process the request.
 */
Event *
SendPriorityManager::getSendPrioritySuccessEvent(
    DataObjectRef dObj,
    NodeRef target)
{
    return new Event(EVENT_TYPE_SEND_PRIORITY_SUCCESS, (void *)new SendPriorityElement(dObj, target));
}

/**
 * Get the event used to dequeue a data object that was unsucessfully sent,
 * and queue up another data object to send.
 * @return The event containing information used by the SendPriorityManager
 *  to process the request.
 */
Event *
SendPriorityManager::getSendPriorityFailureEvent(
    DataObjectRef dObj,
    NodeRef target)
{
    return new Event(EVENT_TYPE_SEND_PRIORITY_FAILURE, (void *)new SendPriorityElement(dObj, target));
}
