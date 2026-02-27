#!/usr/bin/python3.11

import plotly.graph_objects as go

links = [
("Salary","Gross Pay",92076),
("Taxable Reimbursement","Gross Pay",8951),
("Gross Pay","Taxes",32097),
("Gross Pay","Net Pay",44915),

("Net Pay","Checking",44915),
("Roommates","Checking",12677),
("Repayment","Checking",2619),
("Cell Phone Allowance","Checking",219),

("Checking","Credit",5886),

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

("Checking","Housing",21745),
("Housing","Rent",20400),
("Housing","Utilities",1345),

("Checking","Student Loans",16018),
("Student Loans","Monthly Payment",1577),
("Student Loans","Excess Payment",14441),

("Checking","Food & Alcohol",5159),
("Food & Alcohol","Groceries",2700),
("Food & Alcohol","Restaurants",2459),

("Checking","Travel",2721),
("Travel","Lyft & Public Transport",1058),
("Travel","Vacation",1455),
("Travel","Gasoline",208),
("Travel","Unused Transit Card",789),

("Checking","Gaming",4864),
("Checking","Miscellaneous",2650),
("Checking","Personal Loans",3345),
("Checking","Phone",288),
("Checking","Clothing & Shoes",544),
("Checking","Books",1125),

("Checking","Home Improvement",1147),
("Home Improvement","Furniture",479),
("Home Improvement","Garden",168),
("Home Improvement","Air Conditioner",500),

("Checking","Medical Insurance",1210)
]

nodes = sorted(set([s for s, t, v in links] + [t for s, t, v in links]))
node_index = {n:i for i,n in enumerate(nodes)}

sources = [node_index[s] for s,t,v in links]
targets = [node_index[t] for s,t,v in links]
values  = [v for s,t,v in links]



fig = go.Figure(go.Sankey(
    node=dict(label=nodes),
    link=dict(
        source=sources,
        target=targets,
        value=values,
        hovertemplate="%{value:,.0f}<extra></extra>"
    )
))

fig.show()
