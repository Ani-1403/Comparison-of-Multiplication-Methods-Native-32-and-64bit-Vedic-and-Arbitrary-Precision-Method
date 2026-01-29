import streamlit as st
import pandas as pd
import plotly.express as px
import subprocess
import os
import re

# --- App Configuration ---
st.set_page_config(
    page_title="Multiplication Algorithm Dashboard",
    layout="wide"
)

# --- Custom Styling ---
st.markdown("""
<style>
    .stMetric {
        border-radius: 10px;
        padding: 15px;
        background-color: #262730;
    }
    .st-emotion-cache-1r4qj8v {
        border-radius: 10px;
    }
    .stTabs [data-baseweb="tab-list"] {
		gap: 2px;
	}
	.stTabs [data-baseweb="tab"] {
		height: 50px;
        white-space: pre-wrap;
		background-color: #262730;
		border-radius: 4px 4px 0px 0px;
		gap: 1px;
		padding-top: 10px;
		padding-bottom: 10px;
    }
</style>
""", unsafe_allow_html=True)


# --- Core Logic & C++ Integration ---

CPP_FILES = {
    "Native 32-bit": "native_32bit",
    "Native 64-bit": "native_64bit",
    "Long Multiplication": "long_mul",
    "Vedic Lookup": "vedic",
    "Karatsuba": "karatsuba"
}

def safe_float(s):
    try: return float(s.replace(',', ''))
    except (ValueError, TypeError): return None

# --- UPDATED PARSING FUNCTION ---
def parse_native_output(output):
    """
    This function is specifically designed to parse the new output format
    from the updated native_32bit.cpp and native_64bit.cpp files.
    """
    data = {}
    data['raw'] = output
    try:
        # Check for the new error message first
        if "ERROR:" in output:
            data['Product'] = "Error: Input too large"
            data['Cycles'] = 0
            return data

        # Regex to find the new lines for result and cycles
        result_line = re.search(r"Result of last operation: (.*)", output).group(1).strip()
        data['Product'] = result_line.split(' ')[0] # Extract only the number
        
        data['Cycles'] = safe_float(re.search(r"Average cycles per operation: (.*)", output).group(1).strip())
    except AttributeError:
        data['Product'] = "Error"
        data['Cycles'] = 0
    return data

# This parser is for the arbitrary-precision methods
def parse_arbitrary_output(output):
    data = {}
    data['raw'] = output
    try:
        result_line = re.search(r"Result \(last run\): (.*)", output).group(1).strip()
        data['Product'] = result_line.split(' ')[0]
        data['Cycles'] = safe_float(re.search(r"Avg cycles per multiply: (.*)", output).group(1).strip())
    except AttributeError:
        data['Product'] = "Error"
        data['Cycles'] = 0
    return data

def parse_vedic_output(output):
    data = {}
    try:
        round_1_output = output.split("=== Round 1 ===")[1].split("===")[0]
        data['raw'] = round_1_output
        data['Product'] = re.search(r"Product\s*:\s*(.*)", round_1_output).group(1).strip()
        data['Cycles'] = safe_float(re.search(r"Avg total cycles\s*:\s*(.*)", round_1_output).group(1).strip())
        data['Multiplications'] = safe_float(re.search(r"Mul count\s*:\s*(.*)", round_1_output).group(1).strip())
        data['Base Additions'] = safe_float(re.search(r"Add count\s*:\s*(.*)", round_1_output).group(1).strip())
        data['Carry Additions'] = safe_float(re.search(r"Carry Adds\s*:\s*(.*)", round_1_output).group(1).strip())
    except (IndexError, AttributeError):
        data['raw'] = output
        data['Product'] = "Error"
        data['Cycles'] = 0
    return data

def parse_karatsuba_output(output):
    data = {}
    data['raw'] = output
    try:
        data['Product'] = re.search(r"Product: (.*)", output).group(1).strip()
        data['Cycles'] = safe_float(re.search(r"Average CPU Cycles: (.*)", output).group(1).strip())
    except AttributeError:
        data['Product'] = "Error"
        data['Cycles'] = 0
    return data

PARSERS = {
    "Native 32-bit": parse_native_output,
    "Native 64-bit": parse_native_output,
    "Long Multiplication": parse_arbitrary_output, # Uses the old parser
    "Vedic Lookup": parse_vedic_output,
    "Karatsuba": parse_karatsuba_output
}

# --- Algorithm Descriptions ---
ALGO_DESCRIPTIONS = {
    "Native 32-bit": {
        "description": "Uses the CPU's native hardware instruction for 32-bit integers. Extremely fast but limited to numbers below ~4.2 billion.",
        "complexity": "O(1)"
    },
    "Native 64-bit": {
        "description": "Uses the CPU's native hardware instruction for 64-bit integers. Extremely fast but limited to numbers below ~18.4 quintillion.",
        "complexity": "O(1)"
    },
    "Long Multiplication": {
        "description": "A software simulation of the grade-school pen-and-paper method. Works for any size but is inefficient for very large numbers.",
        "complexity": "O(n²)"
    },
    "Vedic Lookup": {
        "description": "A method based on Vedic mathematics that uses lookup tables to simplify multiplication. Works for any size.",
        "complexity": "O(n²)"
    },
    "Karatsuba": {
        "description": "A 'divide and conquer' algorithm that recursively breaks numbers down. Faster than Long Multiplication for large numbers.",
        "complexity": "O(n¹·⁵⁸⁵)"
    }
}


def run_analysis(num1, num2):
    all_results = {}
    for name, exe_name in CPP_FILES.items():
        executable = f"./{exe_name}.exe" if os.name == 'nt' else f"./{exe_name}"
        if not os.path.exists(executable):
            all_results[name] = {'error': f"Executable not found: {executable}"}
            continue
        try:
            input_str = f"{num1}\n{num2}\n"
            if name == "Vedic Lookup":
                input_str = "\n".join([f"{num1}\n{num2}" for _ in range(5)]) + "\n"
            process = subprocess.Popen(
                [executable], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, text=True, creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0
            )
            stdout, stderr = process.communicate(input=input_str, timeout=20)
            if process.returncode != 0 and stderr:
                 all_results[name] = PARSERS[name](stdout)
            else:
                all_results[name] = PARSERS[name](stdout)
        except Exception as e:
            all_results[name] = {'error': str(e)}
    return all_results

# --- Streamlit UI ---

with st.sidebar:
    st.title("Multiplication Dashboard")
    st.markdown("---")
    num1 = st.text_input("First Number", "123456789")
    num2 = st.text_input("Second Number", "987654321")
    analyze_button = st.button("Analyze Performance", type="primary", use_container_width=True)

st.title("Algorithm Performance Analysis")
st.markdown("A dashboard to visually compare native, arbitrary-precision, and advanced multiplication algorithms.")

if analyze_button:
    if not (num1.isdigit() and num2.isdigit()):
        st.error("Please enter valid non-negative integers.")
    else:
        with st.spinner("Running C++ backends... please wait."):
            st.session_state.results = run_analysis(num1, num2)

if 'results' in st.session_state:
    results = st.session_state.results
    
    valid_results = {k: v for k, v in results.items() if 'error' not in v}
    
    if not valid_results:
        st.error("Execution failed for all methods. Please ensure the C++ executables (.exe files) are in the same folder as the Python script and that they are compiled correctly.")
    else:
        df = pd.DataFrame(valid_results).T
        df['Cycles'] = pd.to_numeric(df['Cycles'])

        # --- Create Tabs ---
        tab_dashboard, tab_results, tab_viz, tab_breakdown, tab_logs = st.tabs(["Dashboard", "Results", "Visualizations", "Breakdown Analysis", "Raw Logs"])

        with tab_dashboard:
            st.header("Key Takeaways", divider='rainbow')
            col1, col2 = st.columns(2)
            with col1:
                if not df[df['Cycles'] > 0].empty:
                    fastest_algo = df[df['Cycles'] > 0]['Cycles'].idxmin()
                    st.metric(label="Fastest Algorithm", value=fastest_algo)
                else:
                    st.metric(label="Fastest Algorithm", value="N/A")
            with col2:
                successful_products = df[df['Product'] != "Error"]['Product'].unique()
                if len(successful_products) <= 1:
                    st.metric(label="Product Verification", value="Results Match")
                else:
                    st.metric(label="Product Verification", value="Mismatch Found")

            st.header("Detailed Results", divider='rainbow')
            cols = st.columns(3)
            for i, name in enumerate(CPP_FILES.keys()):
                with cols[i % 3]:
                    with st.container(border=True):
                        st.subheader(name)
                        if name in valid_results:
                            data = valid_results[name]
                            st.metric(label="Average CPU Cycles", value=f"{data.get('Cycles', 0):,.0f}")
                            st.markdown(f"**Method:** {ALGO_DESCRIPTIONS[name]['description']}")
                            st.markdown(f"**Complexity:** `{ALGO_DESCRIPTIONS[name]['complexity']}`")
                        else:
                            st.error("Execution failed.")
                            st.markdown(f"**Error:** {results[name].get('error', 'Unknown error')}")
        
        with tab_results:
            st.header("Final Multiplication Products", divider='rainbow')
            for name, data in valid_results.items():
                st.subheader(name)
                st.text_area("Result", data.get('Product', 'Error'), height=100, key=f"prod_{name}")
            
            st.header("Detailed Comparison Table", divider='rainbow')
            st.dataframe(df, use_container_width=True)

        with tab_viz:
            st.header("Performance Chart", divider='rainbow')
            plot_df = df[df['Cycles'] > 0].sort_values('Cycles')
            if not plot_df.empty:
                fig = px.bar(
                    plot_df, x=plot_df.index, y='Cycles',
                    title="CPU Cycles per Algorithm (Lower is Better)",
                    text='Cycles', color=plot_df.index
                )
                fig.update_traces(texttemplate='%{text:,.0f}', textposition='outside')
                st.plotly_chart(fig, use_container_width=True)
            else:
                st.warning("No valid performance data to plot.")

        with tab_breakdown:
            st.header("Breakdown Analysis", divider='rainbow')
            col1, col2 = st.columns(2)

            with col1:
                st.subheader("CPU Cycle Distribution")
                cycle_df = df[df['Cycles'] > 0]
                if not cycle_df.empty:
                    fig_pie_cycles = px.pie(cycle_df, names=cycle_df.index, values='Cycles', title="Proportion of Total CPU Cycles", hole=.3)
                    st.plotly_chart(fig_pie_cycles, use_container_width=True)
                else:
                    st.warning("No cycle data to display.")

            with col2:
                st.subheader("Vedic Method: Operations Breakdown")
                if "Vedic Lookup" in valid_results and valid_results["Vedic Lookup"].get('Cycles', 0) > 0:
                    vedic_data = valid_results["Vedic Lookup"]
                    op_data = {
                        "Operation Type": ["Multiplications", "Base Additions", "Carry Additions"],
                        "Count": [vedic_data.get("Multiplications", 0), vedic_data.get("Base Additions", 0), vedic_data.get("Carry Additions", 0)]
                    }
                    op_df = pd.DataFrame(op_data)
                    fig_pie_vedic = px.pie(op_df, names='Operation Type', values='Count', title="Vedic Sub-Operation Distribution")
                    st.plotly_chart(fig_pie_vedic, use_container_width=True)
                else:
                    st.warning("Vedic method did not run successfully.")

        with tab_logs:
            st.header("Raw Output Logs", divider='rainbow')
            for name, data in results.items():
                with st.expander(f"Log for: {name}"):
                    if 'error' in data:
                        st.error(data['error'])
                    else:
                        st.code(data.get('raw', 'No raw output available.'), language='text')

else:
    st.info("Enter numbers and click 'Analyze Performance' in the sidebar to begin.")
