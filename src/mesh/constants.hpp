#pragma once

/// MeSH XML data elements
///
/// > See the following references:
/// >   - XML data elements, found [here](https://www.nlm.nih.gov/mesh/xml_data_elements.html)
/// >   - MeSH record types, found [here](https://www.nlm.nih.gov/mesh/intro_record_types.html)
///
/// Note:
///   - We want the absolute bare minimum here as we're not particularly interested in the relationships
///     between MeSH Concepts
///
namespace termspp {
namespace mesh {

/// MeSH XML record fields
///
/// Describes common fields specified as a descendant of MeSH nodes:
///   - The `kStringField` might describe a node that's a grandchild/descendant of the node of interest or
///     it might be specified as a direct child, e.g. in the case of a `<Term />` node
///   - However, we can safely ignore it in the former case by traversing the branch to select the `first_child()`
///
constexpr const char *kStringField = "String";  // char[*]: Inner text of which specifies the unique term

/// MeSH XML node names
///
/// Describes the MeSH nodes of interest to us, i.e. `<${name:-DescriptorRecord} />`
///   - Note that we're intentionally ignoring the relationships defined by `<ConceptRelation />` here as
///     we're not interested in
///
constexpr const char *kRecordSetNode = "DescriptorRecordSet";      // [ Root]: Document root
constexpr const char *kRecordNode    = "DescriptorRecord";         // [Child]: <DescriptorRecordSet/>'s child
constexpr const char *kConcListNode  = "ConceptList";              // [Child]: <DescriptorRecord/>'s child
constexpr const char *kConcNode      = "Concept";                  // [Child]: <ConceptList/>'s child
constexpr const char *kTermListNode  = "TermList";                 // [Child]: <DescriptorRecord/>'s child
constexpr const char *kTermNode      = "Term";                     // [Child]: <TermList/>'s child
constexpr const char *kQualListNode  = "AllowableQualifiersList";  // [Child]: <DescriptRecord/>'s child
constexpr const char *kQualNode      = "AllowableQualifier";       // [Child]: <AllowableQualifiersList/>'s child

/// MeSH XML attributes
///
/// Describes the MeSH attributes of interest to us, i.e. `<SomeNode ${attr:-LexicalTag}="(\w+)" />`
///   - Note that we're mapping these to the `mesh::MeshModifier` enum
///
constexpr const char *kDescClassAttr = "DescriptorClass";         // uint8_t: Specifies whether indexable
constexpr const char *kConcPrefAttr  = "PreferredConceptYN";      // char[1]: Specifies descriptor preference (Y/N)
constexpr const char *kTermConcAttr  = "ConceptPreferredTermYN";  // char[1]: Specifies concept preference (Y/N)
constexpr const char *kTermDescAttr  = "RecordPreferredTermYN";   // char[1]: Specifies descriptor preference (Y/N)
constexpr const char *kTermLexAttr   = "LexicalTag";              // char[3]: Specifies the lexical category (!see refs)

}  // namespace mesh
}  // namespace termspp
