#include <cstdint>

/// MeSH XML data elements
///
/// > See the following references:
/// >   - XML data elements, found [here](https://www.nlm.nih.gov/mesh/xml_data_elements.html#DescriptorRecordSet)
/// >   - MeSH record types, found [here](https://www.nlm.nih.gov/mesh/intro_record_types.html)
///
/// Note:
///   - We want the absolute bare minimum here as we're not interested in using the qualifiers, terms and other
///     mappable data as our ontological terms
///
namespace termspp {
namespace mesh {

/// MeSH unique identifier max length
constexpr int32_t kUidMaxLen = 10;

/// MeSH <DescriptorRecord class="\d+" /> reference value
typedef uint8_t DescriptorClass;
constexpr DescriptorClass kTopicalDescriptorClass = 1;
constexpr DescriptorClass kPublicationTypesClass = 2;
constexpr DescriptorClass kCheckTagClass = 3;
constexpr DescriptorClass kGeographicDescriptorClass = 4;

/// MeSH <DescriptorRecordSet /> node
static constexpr const char *kRecordSetNode = "DescriptorRecordSet"; // Root node

/// MeSH <DescriptorRecord /> node, attr & child prop reference
static constexpr const char *kRecordNode = "DescriptorRecord";

static constexpr const char *kCnctListNode = "ConceptList"; // List of <Concept />

static constexpr const char *kDescClassAttr = "DescriptorClass"; // uint8

static constexpr const char *kDescUIProp = "DescriptorUI";     // char[10]
static constexpr const char *kDescNameProp = "DescriptorName"; // <String char[*] /> (clamped)

/// MeSH <Concept /> child prop reference
static constexpr const char *kConceptNode = "Concept";

static constexpr const char *kCnctPreferAttr = "PreferredConceptYN"; // char[1] of [ Y | N ]

static constexpr const char *kCnctUIProp = "ConceptUI";     // char[10]
static constexpr const char *kCnctNameProp = "ConceptName"; // <String char[*] /> (clamped)

} // namespace mesh
} // namespace termspp
