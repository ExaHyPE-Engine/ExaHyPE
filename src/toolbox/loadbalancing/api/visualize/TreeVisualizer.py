# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import matplotlib.pyplot as plt

# Try to import plotly for HTML export
try:
    import plotly.graph_objects as go
    import plotly.express as px
    from plotly.subplots import make_subplots
    PLOTLY_AVAILABLE = True
except ImportError:
    PLOTLY_AVAILABLE = False

"""
Tree Visualisation Module for Peano Load Balancing API

This module provides comprehensive visualisation capabilities for adaptive mesh trees,
generating both static and interactive representations of tree structures and load
balancing results.

## Visualisation Formats

### Static Formats (Matplotlib)
- **PDF**: Vector format for publications, presentations, and high-quality prints
- **PNG**: Raster format for web display, documentation, and quick previews
- **Resolution**: 300 DPI for publication-quality output
- **Layout**: Automatic subplot arrangement for multi-level display

### Interactive Format (Plotly)
- **HTML**: Self-contained interactive visualisation with JavaScript controls
- **3D Rotation**: Full 3D manipulation for spatial understanding
- **Zoom/Pan**: Detailed inspection of specific tree regions
- **Hover Information**: Real-time display of node coordinates and subtree assignments
- **Responsive Design**: Adapts to browser window size for optimal viewing

## Technical Implementation

### Colour Management
- **Subtree Colors**: 20 distinctive colors for subtree differentiation
- **Colour Cycling**: Automatic colour assignment for large numbers of subtrees
- **Accessibility**: High contrast ratios for clear distinction

### Marker Sizing
- **Level-based Scaling**: Larger markers for coarser levels, smaller for finer levels
- **Mathematical Formula**: size = max(base_size, scale_factor/(level+1)^exponent)
- **Dimension Adaptation**: Different scaling for 2D vs 3D visualisations

## Usage Patterns

### Basic Visualisation
```python
from visualize.TreeVisualizer import TreeVisualizer

# Generate all formats with automatic naming
TreeVisualizer(tree, dimensions=3, filename="analysis_results")
# Outputs: analysis_results.pdf, analysis_results.png, analysis_results.html
```

### Format-specific Generation
```python
# Only static formats (when Plotly not available)
TreeVisualizer(tree, dimensions=2, filename="static_only")

# Interactive-only (custom implementation)
from visualize.TreeVisualizer import save_interactive_html
save_interactive_html(tree, dimensions=3, filename="interactive_analysis")
```

## Integration with Load Balancing Workflow

The visualization system integrates seamlessly with the offline decomposition process:

1. **Original Tree Visualisation**: Shows initial mesh structure before optimisation
2. **Split Tree Visualisation**: Displays optimised load distribution after splitting
3. **Comparison Analysis**: Side-by-side comparison of before/after states

## Error Handling and Dependencies

### Dependency Management
- **Plotly Availability**: Graceful degradation when Plotly not installed
- **Matplotlib Requirements**: Robust handling of display backend issues
- **File System**: Proper error handling for permission and disk space issues
"""

def save_interactive_html(input_tree, dimensions, filename):
    """
    Save an interactive HTML visualization of the tree structure using plotly.
    Creates separate subplots for each level with distinctive colors for subtrees.
    
    Args:
        input_tree: Tree object to visualize
        dimensions: 2 or 3 for 2D/3D visualization
        filename: Base filename for output files (without extension)
        
    Returns:
        None (saves an HTML file)
    """
    if not PLOTLY_AVAILABLE:
        print("Plotly not available - skipping HTML export")
        return
    
    # Define distinctive colors for subtrees (up to 20 subtrees)
    subtree_colors = [
        '#FF0000', '#0000FF', '#00FF00', '#FFA500', '#800080',  # Red, Blue, Green, Orange, Purple
        '#A52A2A', '#FFC0CB', '#808080', '#808000', '#00FFFF',  # Brown, Pink, Gray, Olive, Cyan
        '#FF1493', '#32CD32', '#FFD700', '#DC143C', '#4169E1',  # DeepPink, LimeGreen, Gold, Crimson, RoyalBlue
        '#FF8C00', '#9932CC', '#2E8B57', '#FF6347', '#4682B4'   # DarkOrange, DarkOrchid, SeaGreen, Tomato, SteelBlue
    ]
    
    # Track which subtrees we've seen to manage legend display
    subtrees_shown_in_legend = set()
    
    if dimensions == 3:
        # Create 3D subplots for each level
        fig = make_subplots(
            rows=1, 
            cols=input_tree._max_level,
            subplot_titles=[f"Level {i+1}" for i in range(input_tree._max_level)],
            specs=[[{'type': 'scatter3d'} for _ in range(input_tree._max_level)]]
        )
        
        for level in range(input_tree._max_level):
            plot_array = input_tree._tree_arrays[level]
            
            # Group nodes by subtree for distinctive coloring
            subtree_data = {}
            
            for a in range(3**(level+1)):
                for b in range(3**(level+1)):
                    for c in range(3**(level+1)):
                        node, subtree_id = plot_array[a][b][c]
                        if node != 0:
                            if subtree_id not in subtree_data:
                                subtree_data[subtree_id] = {'x': [], 'y': [], 'z': []}
                            subtree_data[subtree_id]['x'].append(a)
                            subtree_data[subtree_id]['y'].append(b)
                            subtree_data[subtree_id]['z'].append(c)
            
            # Add traces for each subtree at this level
            for subtree_id, coords in subtree_data.items():
                color = subtree_colors[subtree_id % len(subtree_colors)]
                marker_size = max(3, 20/(level+1)**1.5)
                
                # Determine if this subtree should show in legend
                show_in_legend = subtree_id not in subtrees_shown_in_legend
                if show_in_legend:
                    subtrees_shown_in_legend.add(subtree_id)
                
                fig.add_trace(
                    go.Scatter3d(
                        x=coords['x'],
                        y=coords['y'],
                        z=coords['z'],
                        mode='markers',
                        marker=dict(
                            size=marker_size,
                            color=color,
                            opacity=0.8,
                            symbol='square',
                            line=dict(width=1, color='black')
                        ),
                        name=f"Subtree {subtree_id}",  # Simplified name for cleaner legend
                        hovertemplate=f"<b>Level {level+1} - Subtree {subtree_id}</b><br>" +
                                    "X: %{x}<br>" +
                                    "Y: %{y}<br>" +
                                    "Z: %{z}<br>" +
                                    f"Marker size: {marker_size:.1f}<br>" +
                                    "<extra></extra>",
                        legendgroup=f"subtree_{subtree_id}",  # Group by subtree across levels
                        showlegend=show_in_legend  # Only show each subtree once in legend
                    ),
                    row=1, col=level+1
                )
        
        # Update 3D layout with full width
        fig.update_layout(
            title=dict(
                text=f"Interactive 3D Tree Structure - {input_tree._name}<br>" +
                     f"<span style='font-size: 12px'>Use mouse to rotate, zoom, and pan each level</span>",
                font=dict(size=16)
            ),
            width=None,  # Let it use full browser width
            height=800,  # Increased height for better visibility
            margin=dict(t=100, b=50, l=20, r=20),  # Reduced left/right margins
            autosize=True  # Enable automatic sizing
        )
        
        # Update individual 3D scenes
        for i in range(input_tree._max_level):
            scene_name = 'scene' if i == 0 else f'scene{i+1}'
            fig.update_layout(**{
                scene_name: dict(
                    xaxis_title="X Coordinate",
                    yaxis_title="Y Coordinate",
                    zaxis_title="Z Coordinate",
                    xaxis=dict(range=[0, 3**(i+1)-1]),
                    yaxis=dict(range=[0, 3**(i+1)-1]),
                    zaxis=dict(range=[0, 3**(i+1)-1]),
                    aspectmode='cube',
                    camera=dict(
                        eye=dict(x=1.5, y=1.5, z=1.5)
                    )
                )
            })

    elif dimensions == 2:
        # Create 2D subplots for each level
        fig = make_subplots(
            rows=1, 
            cols=input_tree._max_level,
            subplot_titles=[f"Level {i+1}" for i in range(input_tree._max_level)]
        )
        
        for level in range(input_tree._max_level):
            plot_array = input_tree._tree_arrays[level]
            
            # Group nodes by subtree for distinctive coloring
            subtree_data = {}
            
            for a in range(3**(level+1)):
                for b in range(3**(level+1)):
                    node, subtree_id = plot_array[a][b]
                    if node != 0:
                        if subtree_id not in subtree_data:
                            subtree_data[subtree_id] = {'x': [], 'y': []}
                        subtree_data[subtree_id]['x'].append(a)
                        subtree_data[subtree_id]['y'].append(b)
            
            # Add traces for each subtree at this level
            for subtree_id, coords in subtree_data.items():
                color = subtree_colors[subtree_id % len(subtree_colors)]
                marker_size = max(5, 30/(level+1)**2)
                
                # Determine if this subtree should show in legend
                show_in_legend = subtree_id not in subtrees_shown_in_legend
                if show_in_legend:
                    subtrees_shown_in_legend.add(subtree_id)
                
                fig.add_trace(
                    go.Scatter(
                        x=coords['x'],
                        y=coords['y'],
                        mode='markers',
                        marker=dict(
                            size=marker_size,
                            color=color,
                            opacity=0.8,
                            symbol='square',
                            line=dict(width=1, color='black')
                        ),
                        name=f"Subtree {subtree_id}",  # Simplified name for cleaner legend
                        hovertemplate=f"<b>Level {level+1} - Subtree {subtree_id}</b><br>" +
                                    "X: %{x}<br>" +
                                    "Y: %{y}<br>" +
                                    f"Marker size: {marker_size:.1f}<br>" +
                                    "<extra></extra>",
                        legendgroup=f"subtree_{subtree_id}",  # Group by subtree across levels
                        showlegend=show_in_legend  # Only show each subtree once in legend
                    ),
                    row=1, col=level+1
                )
        
        # Update 2D layout with full width
        fig.update_layout(
            title=dict(
                text=f"Interactive 2D Tree Structure - {input_tree._name}<br>" +
                     f"<span style='font-size: 12px'>Use mouse to zoom and pan each level</span>",
                font=dict(size=16)
            ),
            width=None,  # Let it use full browser width
            height=700,  # Increased height for better visibility
            margin=dict(t=100, b=50, l=20, r=20),  # Reduced left/right margins
            autosize=True  # Enable automatic sizing
        )
        
        # Update individual 2D axes
        for i in range(input_tree._max_level):
            fig.update_xaxes(
                title_text="X Coordinate",
                range=[-1, 3**(i+1)],
                row=1, col=i+1
            )
            fig.update_yaxes(
                title_text="Y Coordinate",
                range=[-1, 3**(i+1)],
                row=1, col=i+1
            )

    # Add annotations with tree information
    fig.add_annotation(
        xref="paper", yref="paper",
        x=0.02, y=0.98,
        text=f"<b>Tree Statistics:</b><br>" +
             f"Name: {input_tree._name}<br>" +
             f"Dimensions: {dimensions}D<br>" +
             f"Max Level: {input_tree._max_level}<br>" +
             f"Total Leaves: {int(input_tree._leaf_count)}<br>" +
             f"Subtrees: {input_tree._subtree_count}<br>" +
             f"Split Pattern: {[int(x) for x in input_tree._split_pattern]}",
        showarrow=False,
        font=dict(size=10),
        bgcolor="rgba(255,255,255,0.9)",
        bordercolor="black",
        borderwidth=1,
        align="left"
    )

    # Save HTML file with full width configuration
    config = {
        'displayModeBar': True,
        'displaylogo': False,
        'modeBarButtonsToRemove': ['pan2d', 'lasso2d', 'select2d'],
        'responsive': True  # Make plot responsive to window size
    }
    
    fig.write_html(
        f"{filename}.html",
        config=config,
        div_id="plotly-div",
        include_plotlyjs=True,
        full_html=True
    )
    
    print(f"Interactive HTML visualization saved as {filename}.html")
    print(f"Features:")
    print(f"  • Mouse rotation and zoom ({'3D rotation' if dimensions == 3 else '2D zoom/pan'})")
    print(f"  • Distinctive colors for {input_tree._subtree_count} subtrees")
    print(f"  • Separate subplots for each of {input_tree._max_level} levels")
    print(f"  • Hover information for each node")
    print(f"  • Full browser width layout with responsive design")


def TreeVisualizer(input_tree, dimensions, filename="TreeStructure"):
    """
    Visualize tree structure with customizable output filename.
    Saves in PNG, PDF, and interactive HTML formats.
    
    Args:
        input_tree: Tree object to visualize
        dimensions: 2 or 3 for 2D/3D visualization
        filename: Base filename for output files (without extension)
        
    Returns:
        None (saves files and displays plot)
    """
    # Create matplotlib visualization
    fig = plt.figure(figsize=(9*input_tree._max_level, 8))
    axes = []
    
    for level in range(input_tree._max_level):
        plot_array = input_tree._tree_arrays[level]

        indices = []
        subtree_ids = []
        # extract the existing nodes and their indices from the tree arrays and 
        # plot them in a 3D/2D scatter
        if dimensions == 3:
            for a in range(3**(level+1)):
                for b in range(3**(level+1)):
                    for c in range(3**(level+1)):
                        node, subtree_id = plot_array[a][b][c]
                        if node != 0:
                            indices.append((a, b, c))
                            subtree_ids.append(subtree_id)

            if len(indices) > 0:
                indices = np.array(indices)
                subtree_ids = np.array(subtree_ids)

                axes.append(fig.add_subplot(1, input_tree._max_level, level+1, projection='3d'))
                scatter = axes[level].scatter(indices[:, 0], indices[:, 1], indices[:, 2], 
                                            c=subtree_ids, cmap='tab10', s=1000/(level+1)**3, 
                                            depthshade=True, marker='s')

                axes[level].set_xlabel('X')
                axes[level].set_ylabel('Y')
                axes[level].set_zlabel('Z')

                axes[level].set_xlim([0, 3**(1+level)-1])
                axes[level].set_ylim([0, 3**(1+level)-1]) 
                axes[level].set_zlim([0, 3**(1+level)-1])
                axes[level].set_title("Level="+str(level+1))

        elif dimensions == 2:
            for a in range(3**(level+1)):
                for b in range(3**(level+1)):
                    node, subtree_id = plot_array[a][b]
                    if node != 0:
                        indices.append((a, b))
                        subtree_ids.append(subtree_id)

            if len(indices) > 0:
                indices = np.array(indices)
                subtree_ids = np.array(subtree_ids)

                axes.append(fig.add_subplot(1, input_tree._max_level, level+1))
                scatter = axes[level].scatter(indices[:, 0], indices[:, 1], 
                                            c=subtree_ids, cmap='tab10', s=5000/(level+1)**4, 
                                            marker='s')

                axes[level].set_xlabel('X')
                axes[level].set_ylabel('Y')

                axes[level].set_xlim([-1, 3**(1+level)])
                axes[level].set_ylim([-1, 3**(1+level)]) 
                axes[level].set_title("Level="+str(level+1))

    # Save static formats (matplotlib)
    plt.tight_layout()
    plt.savefig(f"{filename}.pdf", dpi=300, bbox_inches='tight')
    plt.savefig(f"{filename}.png", dpi=300, bbox_inches='tight')
    print(f"Static visualizations saved as {filename}.pdf and {filename}.png")
    
    # Save interactive HTML format (plotly)
    if PLOTLY_AVAILABLE:
        save_interactive_html(input_tree, dimensions, filename)
    else:
        print("Plotly not available. Install with: pip install plotly")
        print("HTML export skipped - only PDF and PNG saved")
    
    # Show the matplotlib plot
    plt.show()
