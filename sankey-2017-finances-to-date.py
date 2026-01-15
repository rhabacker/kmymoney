import plotly.graph_objects as go

nodes = [
  "Salary","Gross Pay","Taxes","Net Pay","Income","Expense","Credit",
  "Retirement Contribution","Employer Contribution",
  "401k","HSA","Previous IRA",
  "Federal Income Tax","State Income Tax","Social Security","Medicare","Unemployment & Disability",
  "Housing","Rent","Utilities",
  "Student Loans","Monthly Payment","Excess Payment",
  "Food & Alcohol","Groceries","Restaurants",
  "Travel","Lyft & Public Transport","Vacation","Gasoline","Unused Transit Card",
  "Gaming","Miscellaneous","Personal Loans","Phone","Clothing & Shoes","Books",
  "Home Improvement","Furniture","Garden","Air Conditioner",
  "Medical Insurance"
]

node_index = {n: i for i, n in enumerate(nodes)}

links = [
  ("Salary","Gross Pay",101027),
  ("Gross Pay","Taxes",32097),
  ("Gross Pay","Net Pay",44915),
  ("Net Pay","Income",60430),
  ("Income","Expense",66316),
  ("Income","Credit",5886),

  ("Salary","Retirement Contribution",14275),
  ("Salary","Employer Contribution",6243),
  ("Retirement Contribution","401k",16240),
  ("Retirement Contribution","HSA",4278),
  ("Employer Contribution","Previous IRA",5500),

  ("Taxes","Federal Income Tax",17066),
  ("Taxes","State Income Tax",6707),
  ("Taxes","Social Security",6010),
  ("Taxes","Medicare",1405),
  ("Taxes","Unemployment & Disability",909),

  ("Expense","Housing",21745),
  ("Housing","Rent",20400),
  ("Housing","Utilities",1345),

  ("Expense","Student Loans",16018),
  ("Student Loans","Monthly Payment",1577),
  ("Student Loans","Excess Payment",14441),

  ("Expense","Food & Alcohol",5159),
  ("Food & Alcohol","Groceries",2700),
  ("Food & Alcohol","Restaurants",2459),

  ("Expense","Travel",2721),
  ("Travel","Lyft & Public Transport",1058),
  ("Travel","Vacation",1455),
  ("Travel","Gasoline",208),
  ("Travel","Unused Transit Card",789),

  ("Expense","Gaming",4864),
  ("Expense","Miscellaneous",2650),
  ("Expense","Personal Loans",3345),
  ("Expense","Phone",288),
  ("Expense","Clothing & Shoes",544),
  ("Expense","Books",1125),

  ("Expense","Home Improvement",1147),
  ("Home Improvement","Furniture",479),
  ("Home Improvement","Garden",168),
  ("Home Improvement","Air Conditioner",500),

  ("Expense","Medical Insurance",1210)
]

fig = go.Figure(go.Sankey(
    node=dict(label=nodes),
    link=dict(
        source=[node_index[s] for s, t, v in links],
        target=[node_index[t] for s, t, v in links],
        value=[v for s, t, v in links]
    )
))

fig.show()
