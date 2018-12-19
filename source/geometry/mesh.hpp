#ifndef MESH_HPP
#define MESH_HPP

#include "general_definitions.hpp"
#include "utilities/heterogeneous_containers.hpp"
#include "utilities/is_vectorized.hpp"
#include "mesh_utilities.hpp"

#include "geometry/element_container.hpp"
#include "geometry/interface_container.hpp"
#include "geometry/boundary_container.hpp"

namespace Geometry {
// Since elements types already come in a tuple. We can use specialization
// to get easy access to the parameter packs for the element and edge types.
template <typename ElementTypeTuple,
          typename InterfaceTypeTuple,
          typename BoundaryTypeTuple,
          typename DistributedBoundaryTypeTuple,
          typename DataSoAType,
          typename BoundarySoAType
 >
class Mesh;

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
class Mesh<std::tuple<Elements...>,
           std::tuple<Interfaces...>,
           std::tuple<Boundaries...>,
           std::tuple<DistributedBoundaries...>,
           DataSoAType,
           BoundarySoAType> {
  public:
    using ElementContainers            = std::tuple<ElementContainer<Elements,DataSoAType>...>;
    using InterfaceContainers          = std::tuple<InterfaceContainer<Interfaces,BoundarySoAType>...>;
    using BoundaryContainers           = std::tuple<BoundaryContainer<Boundaries,BoundarySoAType>...>;
    using DistributedBoundaryContainer = Utilities::HeterogeneousVector<DistributedBoundaries...>;

  private:
    uint p;

    ElementContainers elements;
    InterfaceContainers interfaces;
    BoundaryContainers boundaries;
    DistributedBoundaryContainer distributed_boundaries;

    std::string mesh_name;

  public:
    Mesh() = default;
    Mesh(const uint p) : p(p), elements(container_maker<ElementContainers>::construct_containers(p)) {
        
    }
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&)                  = delete;
    Mesh& operator=(Mesh&&) = default;
    Mesh(Mesh&&)            = default;

    std::string GetMeshName() { return this->mesh_name; }
    void SetMeshName(const std::string& mesh_name) { this->mesh_name = mesh_name; }

    uint GetNumberElements() {
        size_t sz = 0U;
        Utilities::for_each_in_tuple(elements, [&sz](auto& elt_container) {
                sz += elt_container.size();
            });
        return sz;
    }

    uint GetNumberInterfaces() {
        size_t sz = 0U;
        Utilities::for_each_in_tuple(interfaces, [&sz](auto& intfc_container) {
                sz += intfc_container.size();
            });
        return sz;
    }

    uint GetNumberBoundaries() {
        size_t sz = 0U;
        Utilities::for_each_in_tuple(boundaries, [&sz](auto& bdry_container) {
                sz += bdry_container.size();
            });
        return sz;
    }

    uint GetNumberDistributedBoundaries() { return this->distributed_boundaries.size(); }

    void reserve(uint nstages, const std::array<uint,sizeof...(Elements)>& n_elements) {
        uint max_edge_gp = 0u;
        Utilities::for_each_in_tuple(interfaces, [this, &max_edge_gp](auto& interface_container) {
                using intfc_container_t = typename std::decay<decltype(interface_container)>::type;
                using integration_t = typename intfc_container_t::InterfaceType::InterfaceIntegrationType;

                integration_t rule;
                max_edge_gp = std::max(max_edge_gp, rule.GetNumGP(2 * p + 1));
            });

        Utilities::for_each_in_tuple(elements, [nstages, &n_elements, max_edge_gp](auto& element_container) {
                using ContainerType = typename std::remove_reference<decltype(element_container)>::type;
                uint index_of_type = Utilities::index<ContainerType,ElementContainers>::value;
                element_container.reserve(nstages, n_elements[index_of_type], max_edge_gp);
            });
    }

    template <typename InterfaceType>
    void reserve_interfaces(size_t n_interfaces) {
        using InterfaceContainerType = InterfaceContainer<InterfaceType, BoundarySoAType>;

        auto& intface_container =
            std::get<Utilities::index<InterfaceContainerType, InterfaceContainers>::value>(this->interfaces);

        intface_container.reserve(n_interfaces);
    }

    template <typename BoundaryType>
    void reserve_boundaries(size_t n_boundaries) {
        using BoundaryContainerType = BoundaryContainer<BoundaryType, BoundarySoAType>;

        auto& bdry_container =
            std::get<Utilities::index<BoundaryContainerType, BoundaryContainers>::value>(this->boundaries);

        bdry_container.reserve(n_boundaries);
    }

    void finalize_initialization() {
        Utilities::for_each_in_tuple(elements, [](auto& element_container) {
                element_container.finalize_initialization();
            });
    }

    template <typename ElementType, typename... Args>
    void CreateElement(const uint ID, Args&&... args);
    template <typename InterfaceType, typename... Args>
    void CreateInterface(Args&&... args);
    template <typename BoundaryType, typename... Args>
    void CreateBoundary(Args&&... args);
    template <typename DistributedBoundaryType, typename... Args>
    void CreateDistributedBoundary(Args&&... args);

    template <typename F>
    void CallForEachElement(const F& f);
    template <typename F>
    void CallForEachInterface(const F& f);
    template <typename F>
    void CallForEachBoundary(const F& f);
    template <typename F>
    void CallForEachDistributedBoundary(const F& f);

    template <typename ElementType, typename F>
    void CallForEachElementOfType(const F& f);
    template <typename InterfaceType, typename F>
    void CallForEachInterfaceOfType(const F& f);
    template <typename BoundaryType, typename F>
    void CallForEachBoundaryOfType(const F& f);
    template <typename DistributedBoundaryType, typename F>
    void CallForEachDistributedBoundaryOfType(const F& f);

#ifdef HAS_HPX
    template <typename Archive>
    void serialize(Archive& ar, unsigned) {
        // clang-format off
        ar  & mesh_name
            & p;
        // clang-format on
        Utilities::for_each_in_tuple(elements.data, [&ar](auto& element_map) { ar& element_map; });
    }
#endif
};

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
template <typename ElementType, typename... Args>
void Mesh<std::tuple<Elements...>,
          std::tuple<Interfaces...>,
          std::tuple<Boundaries...>,
          std::tuple<DistributedBoundaries...>,
          DataSoAType,
          BoundarySoAType>::CreateElement(const uint ID, Args&&... args) {
    using ElementContainerType = ElementContainer<ElementType, DataSoAType>;

    ElementContainerType& elt_container = std::get<Utilities::index<ElementContainerType, ElementContainers>::value>(this->elements);

    elt_container.CreateElement(ID, std::forward<Args>(args)...);
}

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
template <typename InterfaceType, typename... Args>
void Mesh<std::tuple<Elements...>,
          std::tuple<Interfaces...>,
          std::tuple<Boundaries...>,
          std::tuple<DistributedBoundaries...>,
          DataSoAType,
          BoundarySoAType>::CreateInterface(Args&&... args) {
    using InterfaceContainerType = InterfaceContainer<InterfaceType, BoundarySoAType>;

    InterfaceContainerType& intfc_container = std::get<Utilities::index<InterfaceContainerType, InterfaceContainers>::value>(this->interfaces);

    intfc_container.CreateInterface(std::forward<Args>(args)...);
}

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
template <typename BoundaryType, typename... Args>
void Mesh<std::tuple<Elements...>,
          std::tuple<Interfaces...>,
          std::tuple<Boundaries...>,
          std::tuple<DistributedBoundaries...>,
          DataSoAType,
          BoundarySoAType>::CreateBoundary(Args&&... args) {
    using BoundaryContainerType = BoundaryContainer<BoundaryType, BoundarySoAType>;

    BoundaryContainerType& bdry_container = std::get<Utilities::index<BoundaryContainerType, BoundaryContainers>::value>(this->boundaries);

    bdry_container.CreateBoundary(std::forward<Args>(args)...);
}

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
template <typename DistributedBoundaryType, typename... Args>
void Mesh<std::tuple<Elements...>,
          std::tuple<Interfaces...>,
          std::tuple<Boundaries...>,
          std::tuple<DistributedBoundaries...>,
          DataSoAType,
          BoundarySoAType>::CreateDistributedBoundary(Args&&... args) {
    this->distributed_boundaries.template emplace_back<DistributedBoundaryType>(std::forward<Args>(args)...);
}

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
template <typename F>
void Mesh<std::tuple<Elements...>,
          std::tuple<Interfaces...>,
          std::tuple<Boundaries...>,
          std::tuple<DistributedBoundaries...>,
          DataSoAType,
          BoundarySoAType>::CallForEachElement(const F& f) {
    Utilities::for_each_in_tuple(this->elements, [&f](auto& element_container) {
            using element_t = typename std::decay<decltype(element_container)>::type::ElementType;

            element_container.CallForEachElement(f, Utilities::is_vectorized<F, element_t>{});
    });
}

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
template <typename F>
void Mesh<std::tuple<Elements...>,
          std::tuple<Interfaces...>,
          std::tuple<Boundaries...>,
          std::tuple<DistributedBoundaries...>,
          DataSoAType,
          BoundarySoAType>::CallForEachInterface(const F& f) {
    Utilities::for_each_in_tuple(this->interfaces, [&f](auto& interface_container) {
            interface_container.CallForEachInterface(f, Utilities::is_vectorized<F>{});
    });
}

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
template <typename F>
void Mesh<std::tuple<Elements...>,
          std::tuple<Interfaces...>,
          std::tuple<Boundaries...>,
          std::tuple<DistributedBoundaries...>,
          DataSoAType,
          BoundarySoAType>::CallForEachBoundary(const F& f) {
    Utilities::for_each_in_tuple(this->boundaries, [&f](auto& boundary_container) {
            boundary_container.CallForEachBoundary(f, Utilities::is_vectorized<F>{});
    });
}

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
template <typename F>
void Mesh<std::tuple<Elements...>,
          std::tuple<Interfaces...>,
          std::tuple<Boundaries...>,
          std::tuple<DistributedBoundaries...>,
          DataSoAType,
          BoundarySoAType>::CallForEachDistributedBoundary(const F& f) {
    Utilities::for_each_in_tuple(this->distributed_boundaries.data, [&f](auto& distributed_boundaries_vector) {
        std::for_each(distributed_boundaries_vector.begin(), distributed_boundaries_vector.end(), f);
    });
}

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
template <typename ElementType, typename F>
void Mesh<std::tuple<Elements...>,
          std::tuple<Interfaces...>,
          std::tuple<Boundaries...>,
          std::tuple<DistributedBoundaries...>,
          DataSoAType,
          BoundarySoAType>::CallForEachElementOfType(const F& f) {
    auto& elt_container =
        std::get<Utilities::index<ElementType, ElementContainers>::value>(this->elements);

    using element_t = typename std::decay<decltype(elt_container)>::type::ElementType;

    elt_container.CallForEachElement(f, Utilities::is_vectorized<F, element_t>{});
}

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
template <typename InterfaceType, typename F>
void Mesh<std::tuple<Elements...>,
          std::tuple<Interfaces...>,
          std::tuple<Boundaries...>,
          std::tuple<DistributedBoundaries...>,
          DataSoAType,
          BoundarySoAType>::CallForEachInterfaceOfType(const F& f) {
    auto& intface_container =
        std::get<Utilities::index<InterfaceType, InterfaceContainers>::value>(this->interfaces);

    intface_container.CallForEachInterface(f, Utilities::is_vectorized<F>{});
}

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
template <typename BoundaryType, typename F>
void Mesh<std::tuple<Elements...>,
          std::tuple<Interfaces...>,
          std::tuple<Boundaries...>,
          std::tuple<DistributedBoundaries...>,
          DataSoAType,
          BoundarySoAType>::CallForEachBoundaryOfType(const F& f) {
    auto& bdry_container =
        std::get<Utilities::index<BoundaryType, BoundaryContainers>::value>(this->boundaries);

    bdry_container.CallForEachBoundary(f, Utilities::is_vectorized<F>{});
}

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries, typename DataSoAType, typename BoundarySoAType>
template <typename DistributedBoundaryType, typename F>
void Mesh<std::tuple<Elements...>,
          std::tuple<Interfaces...>,
          std::tuple<Boundaries...>,
          std::tuple<DistributedBoundaries...>,
          DataSoAType,
          BoundarySoAType>::CallForEachDistributedBoundaryOfType(const F& f) {
    auto& dbound_container =
        std::get<Utilities::index<DistributedBoundaryType, typename DistributedBoundaryContainer::TupleType>::value>(
            this->distributed_boundaries.data);

    std::for_each(dbound_container.begin(), dbound_container.end(), f);
}
}

#endif